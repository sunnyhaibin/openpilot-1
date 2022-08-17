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
}
