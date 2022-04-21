#pragma once
#include <functional>
#include <memory>
#include <vector>

namespace ddshop {

class Session;
class SessionConfig;

struct Schedule {
  std::pair<uint8_t, uint8_t> start_time;
  std::pair<uint8_t, uint8_t> stop_time;
};

class Dispatcher {
 public:
  virtual ~Dispatcher() = default;

  virtual void start() = 0;

  virtual void stop() = 0;

  virtual bool initSession(const SessionConfig &) = 0;

  virtual void initBarkNotifier(const std::string &) = 0;

  virtual std::shared_ptr<Session> getSession() = 0;

  virtual void setSchedule(const std::vector<Schedule> &) = 0;

  virtual void onSuccess(std::function<void(const std::string &)>) = 0;

  static std::shared_ptr<Dispatcher> makeDispatcher();
};

}  // namespace ddshop
