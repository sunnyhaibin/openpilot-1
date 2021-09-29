from cereal import car
from common.realtime import DT_CTRL
from common.numpy_fast import interp
from common.realtime import sec_since_boot
from selfdrive.config import Conversions as CV
from selfdrive.car import apply_std_steer_torque_limits
from selfdrive.car.gm import gmcan
from selfdrive.car.gm.values import DBC, AccState, CanBus, CarControllerParams
from opendbc.can.packer import CANPacker

VisualAlert = car.CarControl.HUDControl.VisualAlert


class CarController():
  def __init__(self, dbc_name, CP, VM):
    self.start_time = 0.
    self.apply_steer_last = 0
    self.lka_icon_status_last = (False, False)
    self.steer_rate_limited = False
    self.fcw_count = 0

    self.params = CarControllerParams()

    self.packer_pt = CANPacker(DBC[CP.carFingerprint]['pt'])
    self.packer_obj = CANPacker(DBC[CP.carFingerprint]['radar'])
    self.packer_ch = CANPacker(DBC[CP.carFingerprint]['chassis'])
    
    self.coasting_last_strong_non_cruise_brake_t = 0.
    self.coasting_last_strong_non_cruise_brake_wait_time = 4. # [s]
    self.coasting_strong_brake = 150. # based on max brake in values.py
    self.coasting_lead_dist_BP = [4., 15.] # [m]
    self.coasting_vEgo_apply_brake_BP = [10. * CV.MPH_TO_MS, 15. * CV.MPH_TO_MS]

  def update(self, enabled, CS, frame, actuators,
             hud_v_cruise, hud_show_lanes, hud_show_car, hud_alert):

    P = self.params

    # Send CAN commands.
    can_sends = []

    # STEER
    if (frame % P.STEER_STEP) == 0:
      lkas_enabled = (enabled or CS.pause_long_on_gas_press) and not (CS.out.steerWarning or CS.out.steerError) and CS.out.vEgo > P.MIN_STEER_SPEED
      if lkas_enabled:
        new_steer = int(round(actuators.steer * P.STEER_MAX))
        apply_steer = apply_std_steer_torque_limits(new_steer, self.apply_steer_last, CS.out.steeringTorque, P)
        self.steer_rate_limited = new_steer != apply_steer
      else:
        apply_steer = 0

      self.apply_steer_last = apply_steer
      idx = (frame // P.STEER_STEP) % 4

      can_sends.append(gmcan.create_steering_control(self.packer_pt, CanBus.POWERTRAIN, apply_steer, idx, lkas_enabled))

    if not enabled or CS.pause_long_on_gas_press:
      # Stock ECU sends max regen when not enabled.
      apply_gas = P.MAX_ACC_REGEN
      apply_brake = 0
      if CS.coasting_enabled:
        self.coasting_last_strong_non_cruise_brake_t = sec_since_boot() - self.coasting_last_strong_non_cruise_brake_wait_time
    else:
      apply_gas = int(round(interp(actuators.accel, P.GAS_LOOKUP_BP, P.GAS_LOOKUP_V)))
      if CS.coasting_enabled:
        cur_time = sec_since_boot()
        apply_brake = interp(actuators.accel, P.BRAKE_LOOKUP_BP, P.BRAKE_LOOKUP_V)
        if CS.coasting_long_plan == 'cruise':
          if apply_brake > 0.:
            # compute braking that would be allowed by normal coasting
            apply_brake_coast = interp(cur_time - self.coasting_last_strong_non_cruise_brake_t, [0., self.coasting_last_strong_non_cruise_brake_wait_time], [apply_brake, 0.])
            # now apply brakes based on speed over set cruise speed.
            if CS.coasting_brake_over_speed_enabled:
              apply_brake_over_speed_factor = interp(CS.vEgo - CS.v_cruise_kph * CV.KPH_TO_MS, self.coasting_vEgo_apply_brake_BP, [0., 1.])
              apply_brake = apply_brake * apply_brake_over_speed_factor + (1. - apply_brake_over_speed_factor) * apply_brake_coast
            else:
              apply_brake = apply_brake_coast
            apply_gas = P.MAX_ACC_REGEN
        elif apply_brake > 0. and CS.pcm_acc_status != AccState.STANDSTILL:
          self.coasting_last_strong_non_cruise_brake_t = max(self.coasting_last_strong_non_cruise_brake_t, cur_time - interp(apply_brake, [0., self.coasting_strong_brake], [self.coasting_last_strong_non_cruise_brake_wait_time, 0.]))
        if apply_gas > P.ZERO_GAS:
          self.coasting_last_strong_non_cruise_brake_t = cur_time - self.coasting_last_strong_non_cruise_brake_wait_time
        apply_brake = int(round(apply_brake))
      else:
        apply_brake = int(round(interp(actuators.accel, P.BRAKE_LOOKUP_BP, P.BRAKE_LOOKUP_V)))

    # Gas/regen and brakes - all at 25Hz
    if (frame % 4) == 0:
      idx = (frame // 4) % 4

      if CS.cruiseMain and not enabled and CS.autoHold and CS.autoHoldActive and not CS.out.gasPressed and CS.out.gearShifter == 'drive' and CS.out.vEgo < 0.01 and not CS.regenPaddlePressed:
        # Auto Hold State
        car_stopping = apply_gas < P.ZERO_GAS
        standstill = CS.pcm_acc_status == AccState.STANDSTILL

        at_full_stop = standstill and car_stopping
        near_stop = (CS.out.vEgo < P.NEAR_STOP_BRAKE_PHASE) and car_stopping
        can_sends.append(gmcan.create_friction_brake_command(self.packer_ch, CanBus.CHASSIS, apply_brake, idx, near_stop, at_full_stop))
        CS.autoHoldActivated = True

      else : 
        car_stopping = apply_gas < P.ZERO_GAS
        standstill = CS.pcm_acc_status == AccState.STANDSTILL

        at_full_stop = enabled and standstill and car_stopping
        near_stop = enabled and (CS.out.vEgo < P.NEAR_STOP_BRAKE_PHASE) and car_stopping
        can_sends.append(gmcan.create_friction_brake_command(self.packer_ch, CanBus.CHASSIS, apply_brake, idx, near_stop, at_full_stop))
        CS.autoHoldActivated = False

        # Auto-resume from full stop by resetting ACC control
        acc_enabled = enabled
      
        if standstill and not car_stopping:
          acc_enabled = False
      
        can_sends.append(gmcan.create_gas_regen_command(self.packer_pt, CanBus.POWERTRAIN, apply_gas, idx, acc_enabled, at_full_stop))


    # Send dashboard UI commands (ACC status), 25hz
    if (frame % 4) == 0:
      send_fcw = hud_alert == VisualAlert.fcw
      follow_level = CS.get_follow_level()
      can_sends.append(gmcan.create_acc_dashboard_command(self.packer_pt, CanBus.POWERTRAIN, enabled, 
                                                                 hud_v_cruise * CV.MS_TO_KPH, hud_show_car, follow_level, send_fcw))

    # Radar needs to know current speed and yaw rate (50hz),
    # and that ADAS is alive (10hz)
    time_and_headlights_step = 10
    tt = frame * DT_CTRL

    if frame % time_and_headlights_step == 0:
      idx = (frame // time_and_headlights_step) % 4
      can_sends.append(gmcan.create_adas_time_status(CanBus.OBSTACLE, int((tt - self.start_time) * 60), idx))
      can_sends.append(gmcan.create_adas_headlights_status(self.packer_obj, CanBus.OBSTACLE))

    speed_and_accelerometer_step = 2
    if frame % speed_and_accelerometer_step == 0:
      idx = (frame // speed_and_accelerometer_step) % 4
      can_sends.append(gmcan.create_adas_steering_status(CanBus.OBSTACLE, idx))
      can_sends.append(gmcan.create_adas_accelerometer_speed_status(CanBus.OBSTACLE, CS.out.vEgo, idx))

    if frame % P.ADAS_KEEPALIVE_STEP == 0:
      can_sends += gmcan.create_adas_keepalive(CanBus.POWERTRAIN)

    # Show green icon when LKA torque is applied, and
    # alarming orange icon when approaching torque limit.
    # If not sent again, LKA icon disappears in about 5 seconds.
    # Conveniently, sending camera message periodically also works as a keepalive.
    lka_active = CS.lkas_status == 1
    lka_critical = lka_active and abs(actuators.steer) > 0.9
    lka_icon_status = (lka_active, lka_critical)
    if frame % P.CAMERA_KEEPALIVE_STEP == 0 or lka_icon_status != self.lka_icon_status_last:
      steer_alert = hud_alert in [VisualAlert.steerRequired, VisualAlert.ldw]
      can_sends.append(gmcan.create_lka_icon_command(CanBus.SW_GMLAN, lka_active, lka_critical, steer_alert))
      self.lka_icon_status_last = lka_icon_status

    return can_sends
