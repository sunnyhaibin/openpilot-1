#pragma once

#include "selfdrive/ui/qt/widgets/controls.h"

class SPControlsPanel : public QWidget {
  Q_OBJECT

public:
  explicit SPControlsPanel(QWidget *parent = nullptr);
};

class TorqueFriction : public AbstractControl {
  Q_OBJECT

public:
  TorqueFriction();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;
  Params params;

  void refresh();
};

class TorqueMaxLatAccel : public AbstractControl {
  Q_OBJECT

public:
  TorqueMaxLatAccel();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;
  Params params;

  void refresh();
};

class TorqueDeadzoneDeg : public AbstractControl {
  Q_OBJECT

public:
  TorqueDeadzoneDeg();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;
  Params params;

  void refresh();
};
