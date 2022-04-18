#include "dispatcher_impl.hpp"

#include <chrono>
#include <ctime>
#include <random>

#include "session_impl.hpp"
#include "spdlog/spdlog.h"

namespace ddshop {

DispatcherImpl::DispatcherImpl()
    : running_(false), nopaid_scan_running_(true), force_order_run_(false) {
  unpaid_thread_ = std::thread([this]() { unpaidWorker(); });
}

DispatcherImpl::~DispatcherImpl() { stop(); }

void DispatcherImpl::spawn() {
  running_ = true;
  cart_thread_ = std::thread([this]() { cartWorker(); });
  reserve_time_thread_ = std::thread([this]() { reserveTimeWorker(); });
  order_threads_.clear();
  order_threads_.reserve(ORDER_THREADS_NUM);
  for (int i = 0; i < ORDER_THREADS_NUM; ++i) {
    order_threads_.emplace_back([this, i](){
      orderWorker(i);
    });
  }
}

void DispatcherImpl::start() {
  if (!session_) {
    spdlog::error("Please init session first");
    return;
  }
  if (schedules_.empty() && !running_) {
    spawn();
  }
  while (nopaid_scan_running_) {
    if (!schedules_.empty()) {
      bool should_start = false;
      for (auto &it : schedules_) {
        if (isTimeInPeriod(it)) {
          should_start = true;
          break;
        }
      }
      if (!running_ && should_start) {
        spdlog::info("Schedule start");
        spawn();
      } else if (running_ && !should_start) {
        spdlog::info("Schedule stop");
        pause();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

void DispatcherImpl::stop() {
  nopaid_scan_running_ = false;
  pause();
  if (unpaid_thread_.joinable()) {
    unpaid_thread_.join();
  }
}

void DispatcherImpl::initSession(const SessionConfig &config) {
  session_ = Session::buildSession(config);
  session_->initUser();
}

void DispatcherImpl::cartWorker() {
  std::random_device rd;
  std::mt19937 ra(rd());
  std::uniform_int_distribution<> dist(10000, 20000);
  auto sleep_start = std::chrono::steady_clock::time_point::min();
  uint64_t should_sleep_ms = 0;
  while (running_) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - sleep_start)
                        .count();
    if (duration >= should_sleep_ms) {
      if (!session_->cartCheckAll()) {
        should_sleep_ms = 300;
      } else {
        if (!session_->getCart()) {
          should_sleep_ms = 2000;
        } else {
          should_sleep_ms = dist(ra);
        }
      }
      sleep_start = std::chrono::steady_clock::now();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  spdlog::info("CartWorker stopped");
}

void DispatcherImpl::reserveTimeWorker() {
  std::random_device rd;
  std::mt19937 ra(rd());
  std::uniform_int_distribution<> dist(200, 1000);
  auto sleep_start = std::chrono::steady_clock::time_point::min();
  uint64_t should_sleep_ms = 0;
  while (running_) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - sleep_start)
                        .count();
    if (duration >= should_sleep_ms) {
      session_->refreshReserveTime();
      should_sleep_ms = dist(ra);
      sleep_start = std::chrono::steady_clock::now();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  spdlog::info("ReserveTimeWorker stopped");
}

void DispatcherImpl::orderWorker(int i) {
  std::random_device rd;
  std::mt19937 ra(rd());
  std::uniform_int_distribution<> dist(100, 300);
  auto sleep_start = std::chrono::steady_clock::time_point::min();
  uint64_t should_sleep_ms = 0;
  spdlog::info("Order worker {} started", i);
  while (running_) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - sleep_start)
                        .count();
    if (force_order_run_) {
      force_order_run_ = false;
      duration = std::numeric_limits<long>::max();
    }
    if (duration >= should_sleep_ms) {
      std::vector<std::pair<uint64_t, uint64_t>> reserve_times;
      session_->getReserveTime(reserve_times);
      if (!reserve_times.empty()) {
        std::uniform_int_distribution<size_t> dist_idx(
            0, reserve_times.size() - 1);
        size_t idx = dist_idx(ra);
        spdlog::debug("Order worker {} trying {} reserve time {}-{}", i, idx,
                      reserve_times[idx].first, reserve_times[idx].second);
        ddshop::Order order;
        int code = -1;
        if (session_->checkOrder(reserve_times[idx], order, code)) {
          if (session_->doOrder(order, code)) {
            spdlog::info("Order worker {} success", i);
            notify("抢菜成功！快去支付！！");
          } else {
            if (code == 5001 || code == 5003 || code == 5004) {
              force_order_run_ = true;
            }
            spdlog::warn("Order worker {} do order failed", i);
          }
        } else {
          if (code == 5001 || code == 5003 || code == 5004) {
            force_order_run_ = true;
          }
          spdlog::warn("Order worker {} check order failed", i);
        }
      }
      should_sleep_ms = dist(ra);
      sleep_start = std::chrono::steady_clock::now();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  spdlog::info("Order worker {} stopped", i);
}

void DispatcherImpl::initBarkNotifier(const std::string &bark_id) {
  bark_notifier_ = notification::BarkNotifier::makeBarkNotifier(
      std::forward<const std::string &>(bark_id));
}

void DispatcherImpl::notify(const std::string &msg) {
  if (bark_notifier_) {
    spdlog::info("Sending bark notification {}", msg);
    bark_notifier_->notify(std::forward<const std::string &>(msg));
  }
}

void DispatcherImpl::unpaidWorker() {
  std::random_device rd;
  std::mt19937 ra(rd());
  std::uniform_int_distribution<> dist(55, 65);
  auto sleep_start = std::chrono::steady_clock::time_point::min();
  uint64_t should_sleep_ms = 0;
  while (nopaid_scan_running_) {
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - sleep_start)
                        .count();
    if (session_ && duration >= should_sleep_ms) {
      if (session_->hasUnpaidOrder()) {
        notify("您有未支付的订单，请前往支付！！");
      }
      should_sleep_ms = dist(ra) * 1000;
      sleep_start = std::chrono::steady_clock::now();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  spdlog::info("UnpaidWorker stopped");
}

void DispatcherImpl::setSchedule(const std::vector<Schedule> &schedules) {
  schedules_ = schedules;
}

void DispatcherImpl::pause() {
  if (running_) {
    running_ = false;
  }
  if (cart_thread_.joinable()) {
    cart_thread_.join();
  }
  if (reserve_time_thread_.joinable()) {
    reserve_time_thread_.join();
  }
  for (auto &it : order_threads_) {
    if (it.joinable()) {
      it.join();
    }
  }
  order_threads_.clear();
}

bool DispatcherImpl::isTimeInPeriod(const Schedule &schedule) {
  time_t current_time;
  struct tm *local_time;

  time(&current_time);
  local_time = localtime(&current_time);

  auto now_secs = local_time->tm_hour * 3600 + local_time->tm_min * 60;
  auto start_secs =
      schedule.start_time.first * 3600 + schedule.start_time.second * 60;
  auto stop_secs =
      schedule.stop_time.first * 3600 + schedule.stop_time.second * 60;

  if (start_secs == stop_secs) {
    spdlog::error("Invalid schedule");
    return false;
  } else if (start_secs < stop_secs) {
    return (now_secs >= start_secs) && (now_secs < stop_secs);
  } else {
    return (now_secs >= 0 && now_secs < stop_secs) || (now_secs >= start_secs);
  }
}

std::shared_ptr<Dispatcher> Dispatcher::makeDispatcher() {
  return std::make_shared<DispatcherImpl>();
}

}  // namespace ddshop