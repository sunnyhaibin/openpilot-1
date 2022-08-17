#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>

#include "selfdrive/ui/qt/api.h"
#include "selfdrive/ui/qt/offroad/sunnypilot_settings.h"
#include "selfdrive/ui/qt/widgets/input.h"

#include "selfdrive/ui/ui.h"

SPControlsPanel::SPControlsPanel(QWidget* parent) : QWidget(parent) {

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  QWidget* madsMainControl = new QWidget(this);
  QVBoxLayout* madsSubControl = new QVBoxLayout(this);
  QWidget* customTorqueMain = new QWidget(this);
  QVBoxLayout* customTorqueSub = new QVBoxLayout(this);

  ParamControl *madsControl = new ParamControl("EnableMads",
                                               "Enable M.A.D.S.",
                                               "Enable the beloved M.A.D.S. feature. Disable toggle to revert back to stock openpilot engagement/disengagement.",
                                               "../assets/offroad/icon_openpilot.png");
  madsSubControl->setContentsMargins(QMargins());
  madsSubControl->addWidget(horizontal_line());
  madsSubControl->addWidget(new ParamControl("DisengageLateralOnBrake",
                                             "Disengage Lateral On Brake Pedal",
                                             "When enabled, pressing the brake pedal will disengage lateral control on sunnypilot.",
                                             "../assets/offroad/icon_openpilot.png"));
  madsSubControl->addWidget(horizontal_line());
  madsSubControl->addWidget(new ParamControl("AccMadsCombo",
                                             "Enable ACC+MADS with RES+/SET-",
                                             "Engage both M.A.D.S. and ACC with a single press of RES+ or SET- button.\nNote: Once M.A.D.S. is engaged via this mode, it will remain engaged until it is manually disabled via the M.A.D.S. button or car shut off.",
                                             "../assets/offroad/icon_openpilot.png"));
  madsSubControl->addWidget(horizontal_line());
  madsMainControl->setLayout(madsSubControl);
  QObject::connect(madsControl, &ToggleControl::toggleFlipped, [=](bool state) {
    if (state) {
      madsMainControl->show();
    } else {
      madsMainControl->hide();
    }
  });

  main_layout->addWidget(madsControl);
  main_layout->addWidget(madsMainControl);

  if (!Params().getBool("EnableMads"))
    madsMainControl->hide();

  ParamControl *customTorqueLateral = new ParamControl("CustomTorqueLateral",
                                                       "Torque Lateral Control Live Tune",
                                                       "Enables live tune for Torque lateral control.",
                                                       "../assets/offroad/icon_calibration.png");
  QObject::connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    customTorqueLateral->setEnabled(offroad);
  });
  customTorqueSub->setContentsMargins(QMargins());
  customTorqueSub->addWidget(horizontal_line());
  customTorqueSub->addWidget(new TorqueFriction());
  customTorqueSub->addWidget(horizontal_line());
  customTorqueSub->addWidget(new TorqueMaxLatAccel());
  customTorqueSub->addWidget(horizontal_line());
  customTorqueSub->addWidget(new TorqueDeadzoneDeg());
  customTorqueMain->setLayout(customTorqueSub);
  QObject::connect(customTorqueLateral, &ToggleControl::toggleFlipped, [=](bool state) {
    if (state) {
      customTorqueMain->show();
    } else {
      customTorqueMain->hide();
    }
  });
  main_layout->addWidget(horizontal_line());
  main_layout->addWidget(customTorqueLateral);
  main_layout->addWidget(customTorqueMain);
  if (!Params().getBool("CustomTorqueLateral"))
    customTorqueMain->hide();
}

TorqueFriction::TorqueFriction() : AbstractControl("Friction", "Adjust Friction for the Torque Lateral Controller", "../assets/offroad/icon_blank.png") {

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("TorqueFriction"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 1 ) {
      value = 1;
    }
    QString values = QString::number(value);
    params.put("TorqueFriction", values.toStdString());
    refresh();
  });

  QObject::connect(&btnplus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("TorqueFriction"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 50 ) {
      value = 50;
    }
    QString values = QString::number(value);
    params.put("TorqueFriction", values.toStdString());
    refresh();
  });
  refresh();
}

void TorqueFriction::refresh() {
  auto strs = QString::fromStdString(params.get("TorqueFriction"));
  int valuei = strs.toInt();
  float valuef = valuei * 0.01;
  QString valuefs = QString::number(valuef);
  label.setText(QString::fromStdString(valuefs.toStdString()));
  btnminus.setText("-");
  btnplus.setText("+");
}

TorqueMaxLatAccel::TorqueMaxLatAccel() : AbstractControl("Max Lateral Acceleration", "Adjust Max Lateral Acceleration for the Torque Lateral Controller", "../assets/offroad/icon_blank.png") {

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("TorqueMaxLatAccel"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 1 ) {
      value = 1;
    }
    QString values = QString::number(value);
    params.put("TorqueMaxLatAccel", values.toStdString());
    refresh();
  });

  QObject::connect(&btnplus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("TorqueMaxLatAccel"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 500 ) {
      value = 500;
    }
    QString values = QString::number(value);
    params.put("TorqueMaxLatAccel", values.toStdString());
    refresh();
  });
  refresh();
}

void TorqueMaxLatAccel::refresh() {
  auto strs = QString::fromStdString(params.get("TorqueMaxLatAccel"));
  int valuei = strs.toInt();
  float valuef = valuei * 0.01;
  QString valuefs = QString::number(valuef);
  label.setText(QString::fromStdString(valuefs.toStdString()));
  btnminus.setText("-");
  btnplus.setText("+");
}

TorqueDeadzoneDeg::TorqueDeadzoneDeg() : AbstractControl("Steering Angle Deadzone Degree", "Adjust Steering Angle Deadzone Degree for the Torque Lateral Controller", "../assets/offroad/icon_blank.png") {

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
  label.setStyleSheet("color: #e0e879");
  hlayout->addWidget(&label);

  btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btnminus.setFixedSize(150, 100);
  btnplus.setFixedSize(150, 100);
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("TorqueDeadzoneDeg"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 0 ) {
      value = 0;
    }
    QString values = QString::number(value);
    params.put("TorqueDeadzoneDeg", values.toStdString());
    refresh();
  });

  QObject::connect(&btnplus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("TorqueDeadzoneDeg"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 50 ) {
      value = 50;
    }
    QString values = QString::number(value);
    params.put("TorqueDeadzoneDeg", values.toStdString());
    refresh();
  });
  refresh();
}

void TorqueDeadzoneDeg::refresh() {
  auto strs = QString::fromStdString(params.get("TorqueDeadzoneDeg"));
  int valuei = strs.toInt();
  float valuef = valuei * 0.01;
  QString valuefs = QString::number(valuef);
  label.setText(QString::fromStdString(valuefs.toStdString()));
  btnminus.setText("-");
  btnplus.setText("+");
}
