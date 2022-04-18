#pragma once
#include <atomic>
#include <condition_variable>
#include <thread>

#include "ddshop/dispatcher.hpp"
#include "notification/bark.hpp"

namespace ddshop {

class DispatcherImpl : public Dispatcher {
 public:
  DispatcherImpl();

  ~DispatcherImpl() override;

  void start() override;

  void stop() override;

  std::shared_ptr<Session> getSession() override { return session_; };

  void initSession(const SessionConfig &) override;

  void initBarkNotifier(const std::string &) override;

  void setSchedule(const std::vector<Schedule> &) override;

 private:
  std::shared_ptr<Session> session_;
  std::shared_ptr<notification::BarkNotifier> bark_notifier_;
  std::atomic<bool> running_;
  std::atomic<bool> nopaid_scan_running_;
  std::atomic<bool> force_order_run_;
  std::thread cart_thread_;
  std::thread reserve_time_thread_;
  std::thread order_thread_;
  std::thread unpaid_thread_;

  std::vector<Schedule> schedules_;

  void cartWorker();
  void reserveTimeWorker();
  void orderWorker();
  void unpaidWorker();

  void pause();

  bool isTimeInPeriod(const Schedule &schedule);

  void notify(const std::string &);
};

}  // namespace ddshop
