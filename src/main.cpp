#include <csignal>
#include <fstream>

#include "ddshop/dispatcher.hpp"
#include "ddshop/session.hpp"
#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

std::shared_ptr<ddshop::Dispatcher> dispatcher;

void registerSignalHandler() {
  auto handler = [](int sig) {
    if (dispatcher) {
      dispatcher->stop();
    }
  };
  std::signal(SIGINT, handler);
  std::signal(SIGTERM, handler);
  std::signal(SIGHUP, handler);
}

int main(int argc, char **argv) {
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);

  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      "ddshop.log", 30000000, 3);
  file_sink->set_level(spdlog::level::debug);

  std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
  spdlog::init_thread_pool(1024, 1);
  auto logger = std::make_shared<spdlog::async_logger>(
      "ddshop", sinks.begin(), sinks.end(), spdlog::thread_pool(),
      spdlog::async_overflow_policy::block);
  logger->set_level(spdlog::level::debug);
  spdlog::set_default_logger(logger);

  if (argc != 2) {
    printf("Usage: ddshop_cpp xxx.json");
    return -1;
  }

  std::ifstream fin(argv[1]);
  if (!fin.good()) {
    spdlog::error("Failed to open config file");
    return -1;
  }
  std::string config_json_str((std::istreambuf_iterator<char>(fin)),
                              std::istreambuf_iterator<char>());
  auto config_json = nlohmann::json::parse(config_json_str, nullptr, false);
  if (config_json.is_discarded()) {
    spdlog::error("Failed to parse config file");
    return -1;
  }
  if (!config_json.contains("cookie")) {
    spdlog::error("Config file must contain cookie");
    return -1;
  }

  ddshop::SessionConfig config{};
  config.cookie = config_json["cookie"];
  if (config_json.contains("channel")) {
    if (config_json["channel"] == "app" || config_json["channel"] == "APP") {
      spdlog::info("Using app channel");
      config.channel = ddshop::Channel::APP;
    } else if (config_json["channel"] == "applet" ||
               config_json["channel"] == "APPLET") {
      spdlog::info("Using applet channel");
      config.channel = ddshop::Channel::APPLET;
    }
  }
  if (config_json.contains("pay_type")) {
    if (config_json["pay_type"] == "alipay" ||
        config_json["pay_type"] == "ALIPAY") {
      spdlog::info("Using alipay");
      config.pay_type = ddshop::PayType::ALIPAY;
    } else if (config_json["pay_type"] == "wechat" ||
               config_json["pay_type"] == "WECHAT") {
      spdlog::info("Using wechat");
      config.pay_type = ddshop::PayType::WECHAT;
    }
  }

  registerSignalHandler();

  dispatcher = ddshop::Dispatcher::makeDispatcher();
  dispatcher->initSession(config);
  if (config_json.contains("bark_id")) {
    dispatcher->initBarkNotifier(config_json["bark_id"]);
  }

  if (config_json.contains("schedules")) {
    std::vector<ddshop::Schedule> schedules;
    for (auto &it : config_json["schedules"]) {
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
      spdlog::info("Schedule task {:02d}:{:02d} to {:02d}:{:02d}",
                   tmp.start_time.first, tmp.start_time.second,
                   tmp.stop_time.first, tmp.stop_time.second);
      schedules.emplace_back(std::move(tmp));
    }
    if (!schedules.empty()) {
      dispatcher->setSchedule(schedules);
    }
  }

  std::vector<ddshop::Address> addresses;
  dispatcher->getSession()->getAddresses(addresses);

  auto address = addresses.back();
  if (config_json.contains("address_keyword")) {
    for (auto &it : addresses) {
      if (it.address.find(config_json["address_keyword"]) !=
          std::string::npos) {
        spdlog::info("Filtered address {}", it.address);
        address = it;
        break;
      }
    }
  }
  dispatcher->getSession()->setAddress(address);

  dispatcher->start();
  dispatcher.reset();
  return 0;
}
