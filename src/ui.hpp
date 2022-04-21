#include <QMainWindow>
#include <QSound>
#include <memory>

#include "ddshop/dispatcher.hpp"
#include "ddshop/session.hpp"
#include "ui_main.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(int argc, char **argv);

 private:
  Ui_MainWindow ui_;
  std::vector<ddshop::Address> addresses_;
  std::vector<ddshop::Schedule> schedules_;
  std::shared_ptr<ddshop::Dispatcher> dispatcher_;
  QSound sound_;
  void parseConfig(const std::string &);

 signals:
  void onSuccess(const QString &);

 public slots:
  void btnLoginClick();
  void btnStartClick();
  void btnAddClick();
  void btnDelClick();
  void play(const QString &);
  void log(const QString &);
};
