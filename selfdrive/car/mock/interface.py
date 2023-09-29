#!/usr/bin/env python3
from typing import Dict, Tuple, List

from cereal import car
import cereal.messaging as messaging
from openpilot.selfdrive.car.interfaces import CarInterfaceBase

# mocked car interface for dashcam mode
class CarInterface(CarInterfaceBase):
  def __init__(self, CP, CarController, CarState):
    super().__init__(CP, CarController, CarState)

    self.speed = 0.
    self.sm = messaging.SubMaster(['gpsLocation', 'gpsLocationExternal'])

  @staticmethod
  def _get_params(ret: car.CarParams, candidate: str, fingerprint: Dict[int, Dict[int, int]],
                  car_fw: List[car.CarParams.CarFw], experimental_long: bool, docs: bool) -> car.CarParams:
    ret.carName = "mock"
    ret.mass = 1700.
    ret.wheelbase = 2.70
    ret.centerToFront = ret.wheelbase * 0.5
    ret.steerRatio = 13.
    return ret

  def _update(self, c: car.CarControl) -> car.CarState:
    self.sm.update(0)
    gps_sock = 'gpsLocationExternal' if self.sm.rcv_frame['gpsLocationExternal'] > 1 else 'gpsLocation'

    ret = car.CarState.new_message()
    ret.vEgo = self.sm[gps_sock].speed
    ret.vEgoRaw = self.sm[gps_sock].speed

    return ret

  def apply(self, c: car.CarControl, now_nanos: int) -> Tuple[car.CarControl.Actuators, List[bytes]]:
    actuators = car.CarControl.Actuators.new_message()
    return actuators, []
