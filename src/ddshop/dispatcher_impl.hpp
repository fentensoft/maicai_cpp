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

  bool initSession(const SessionConfig &) override;

  void initBarkNotifier(const std::string &) override;

  void setSchedule(const std::vector<Schedule> &) override;

  void onSuccess(std::function<void(const std::string &)>) override;

 private:
  const uint8_t ORDER_THREADS_NUM = 2;

  std::shared_ptr<Session> session_;
  std::shared_ptr<notification::BarkNotifier> bark_notifier_;
  std::atomic<bool> running_;
  std::atomic<bool> nopaid_scan_running_;
  std::atomic<bool> force_order_run_;
  std::thread cart_thread_;
  std::thread reserve_time_thread_;
  std::vector<std::thread> order_threads_;
  std::thread unpaid_thread_;
  std::thread schedule_thread_;

  std::vector<Schedule> schedules_;

  std::function<void(const std::string &)> success_cb_;

  void cartWorker();
  void reserveTimeWorker();
  void orderWorker(int i);
  void unpaidWorker();
  void scheduleWorker();

  void spawn();
  void pause();

  static bool isTimeInPeriod(const Schedule &schedule);

  void notify(const std::string &);
};

}  // namespace ddshop
