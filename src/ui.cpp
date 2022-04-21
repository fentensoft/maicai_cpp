#include "ui.hpp"

#include <QInputDialog>
#include <QMessageBox>
#include <fstream>
#include <regex>

#include "fmt/core.h"
#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/sinks/qt_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

void MainWindow::parseConfig(const std::string& config_file_path) {
  std::ifstream fin(config_file_path);
  if (!fin.good()) {
    return;
  }
  std::string config_json_str((std::istreambuf_iterator<char>(fin)),
                              std::istreambuf_iterator<char>());
  auto config_json = nlohmann::json::parse(config_json_str, nullptr, false);
  if (config_json.is_discarded()) {
    spdlog::error("Failed to parse config file");
    return;
  }

  if (config_json.contains("cookie")) {
    ui_.editCookie->setText(
        QString::fromStdString(config_json["cookie"].get<std::string>()));
  }
  if (config_json.contains("bark_id")) {
    ui_.editBarkId->setText(
        QString::fromStdString(config_json["bark_id"].get<std::string>()));
  }
  if (config_json.contains("channel")) {
    if (config_json["channel"] == "app" || config_json["channel"] == "APP") {
      ui_.comboChannel->setCurrentText("App");
    } else if (config_json["channel"] == "applet" ||
               config_json["channel"] == "APPLET") {
      ui_.comboChannel->setCurrentText("Applet");
    }
  }
  if (config_json.contains("pay_type")) {
    if (config_json["pay_type"] == "alipay" ||
        config_json["pay_type"] == "ALIPAY") {
      ui_.comboPay->setCurrentText("Alipay");
    } else if (config_json["pay_type"] == "wechat" ||
               config_json["pay_type"] == "WECHAT") {
      ui_.comboPay->setCurrentText("Wechat");
    }
  }
  if (config_json.contains("schedules")) {
    schedules_.clear();
    ui_.listSched->clear();
    for (auto& it : config_json["schedules"]) {
      if (it["start"].size() != 2 || it["stop"].size() != 2 ||
          it["start"][0] < 0 || it["start"][0] > 23 || it["stop"][0] < 0 ||
          it["stop"][0] > 23 || it["start"][1] < 0 || it["start"][1] > 59 ||
          it["stop"][1] < 0 || it["stop"][1] > 59 ||
          (it["start"][0] == it["stop"][0] &&
           it["start"][1] == it["stop"][1])) {
        spdlog::error("Invalid schedule");
        continue;
      }
      ddshop::Schedule tmp;
      tmp.start_time = std::make_pair(it["start"][0].get<uint64_t>(),
                                      it["start"][1].get<uint64_t>());
      tmp.stop_time = std::make_pair(it["stop"][0].get<uint64_t>(),
                                     it["stop"][1].get<uint64_t>());
      ui_.listSched->addItem(QString::fromStdString(fmt::format(
          "{:02d}:{:02d} - {:02d}:{:02d}", tmp.start_time.first,
          tmp.start_time.second, tmp.stop_time.first, tmp.stop_time.second)));
      schedules_.emplace_back(std::move(tmp));
    }
  }
}

void MainWindow::btnAddClick() {
  bool ok;
  auto text = QInputDialog::getText(this, "Add schedule",
                                    "Schedule time ( in format HH:SS-HH:SS )",
                                    QLineEdit::Normal, "", &ok)
                  .toStdString();
  if (ok) {
    std::regex r(R"(^(\d{2}):(\d{2})\s*\-\s*(\d{2}):(\d{2})$)");
    std::smatch match;
    if (std::regex_search(text, match, r) && match.size() == 5) {
      ddshop::Schedule tmp{};
      tmp.start_time.first = std::stoi(match[1].str());
      tmp.start_time.second = std::stoi(match[2].str());
      tmp.stop_time.first = std::stoi(match[3].str());
      tmp.stop_time.second = std::stoi(match[4].str());
      if (tmp.start_time.first > 23 || tmp.stop_time.first > 23 ||
          tmp.start_time.second > 59 || tmp.stop_time.second > 59 ||
          (tmp.start_time.first == tmp.stop_time.first &&
           tmp.start_time.second == tmp.stop_time.second)) {
        spdlog::error("Invalid schedule time");
      } else {
        spdlog::info("Adding schedule {:02d}:{:02d}-{:02d}:{:02d}",
                     tmp.start_time.first, tmp.start_time.second,
                     tmp.stop_time.first, tmp.stop_time.second);
        ui_.listSched->addItem(QString::fromStdString(fmt::format(
            "{:02d}:{:02d} - {:02d}:{:02d}", tmp.start_time.first,
            tmp.start_time.second, tmp.stop_time.first, tmp.stop_time.second)));
        schedules_.emplace_back(std::move(tmp));
      }
    } else {
      QMessageBox::warning(this, "Warning", "Invalid string");
    }
  }
}

void MainWindow::btnDelClick() {
  if (ui_.listSched->currentRow() >= 0 &&
      ui_.listSched->currentRow() < schedules_.size()) {
    schedules_.erase(schedules_.begin() + ui_.listSched->currentRow());
    ui_.listSched->takeItem(ui_.listSched->currentRow());
  }
}

void MainWindow::log(const QString& str) { ui_.textLog->append(str); }

void MainWindow::play(const QString& msg) {
  if (sound_.isFinished()) {
    sound_.play();
    QMessageBox::information(this, "Success", msg);
    sound_.stop();
  }
}

MainWindow::MainWindow(int argc, char** argv)
    : QMainWindow(nullptr),
      ui_(),
      sound_("/home/fentensoft/Downloads/notice.wav") {
  ui_.setupUi(this);

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);

  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      "ddshop.log", 30000000, 3);
  file_sink->set_level(spdlog::level::debug);

  auto qt_sink = std::make_shared<spdlog::sinks::qt_sink_mt>(this, "log");

  std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink, qt_sink};
  spdlog::init_thread_pool(1024, 1);
  auto logger = std::make_shared<spdlog::async_logger>(
      "ddshop", sinks.begin(), sinks.end(), spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);
  logger->set_level(spdlog::level::debug);
  spdlog::set_default_logger(logger);

  if (argc == 2) {
    parseConfig(argv[1]);
  }
  connect(ui_.btnLogin, &QPushButton::clicked, this,
          &MainWindow::btnLoginClick);
  connect(ui_.btnStart, &QPushButton::clicked, this,
          &MainWindow::btnStartClick);
  connect(ui_.btnAdd, &QPushButton::clicked, this, &MainWindow::btnAddClick);
  connect(ui_.btnDelete, &QPushButton::clicked, this, &MainWindow::btnDelClick);
  connect(this, &MainWindow::onSuccess, this, &MainWindow::play,
          Qt::QueuedConnection);
  show();
}

void MainWindow::btnLoginClick() {
  if (dispatcher_ != nullptr) {
    return;
  }
  if (ui_.editCookie->text().isEmpty()) {
    QMessageBox::warning(this, "Warning", "Cookie could not be empty");
    return;
  }
  ddshop::SessionConfig config{};
  config.cookie = ui_.editCookie->text().toStdString();
  if (ui_.comboPay->currentText() == "Alipay") {
    config.pay_type = ddshop::PayType::ALIPAY;
  } else if (ui_.comboPay->currentText() == "Wechat") {
    config.pay_type = ddshop::PayType::WECHAT;
  }
  if (ui_.comboChannel->currentText() == "App") {
    config.channel = ddshop::Channel::APP;
  } else if (ui_.comboChannel->currentText() == "Applet") {
    config.channel = ddshop::Channel::APPLET;
  }
  dispatcher_ = ddshop::Dispatcher::makeDispatcher();
  if (dispatcher_->initSession(config)) {
    addresses_.clear();
    dispatcher_->getSession()->getAddresses(addresses_);
    if (addresses_.empty()) {
      QMessageBox::warning(this, "Warning", "No available address found");
    } else {
      ui_.btnLogin->setDisabled(true);
      ui_.editCookie->setDisabled(true);
      ui_.editBarkId->setDisabled(true);
      ui_.comboChannel->setDisabled(true);
      ui_.comboPay->setDisabled(true);

      ui_.btnStart->setEnabled(true);

      if (!ui_.editBarkId->text().isEmpty()) {
        dispatcher_->initBarkNotifier(ui_.editBarkId->text().toStdString());
      }

      dispatcher_->onSuccess([this](const std::string& msg) {
        emit(onSuccess(QString::fromStdString(msg)));
      });

      ui_.comboAddress->clear();
      for (auto& it : addresses_) {
        ui_.comboAddress->addItem(QString::fromStdString(it.address));
      }
    }
  } else {
    QMessageBox::warning(this, "Warning", "Login failed, check your cookie");
  }
}

void MainWindow::btnStartClick() {
  if (!ui_.comboAddress->currentText().isEmpty()) {
    if (ui_.btnStart->text() == "Start") {
      dispatcher_->getSession()->setAddress(
          addresses_.at(ui_.comboAddress->currentIndex()));
      if (!schedules_.empty()) {
        dispatcher_->setSchedule(schedules_);
      }

      dispatcher_->start();

      ui_.comboAddress->setDisabled(true);
      ui_.btnAdd->setDisabled(true);
      ui_.btnDelete->setDisabled(true);
      ui_.listSched->setDisabled(true);
      ui_.btnStart->setText("Stop");
    } else {
      dispatcher_->stop();

      ui_.comboAddress->setDisabled(false);
      ui_.btnAdd->setDisabled(false);
      ui_.btnDelete->setDisabled(false);
      ui_.listSched->setDisabled(false);
      ui_.btnStart->setText("Start");
    }
  }
}
