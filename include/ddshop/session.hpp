#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

namespace ddshop {

enum class Channel : uint8_t { APP = 3, APPLET = 4 };
enum class PayType : uint8_t { ALIPAY = 2, WECHAT = 4 };

struct SessionConfig {
  std::string cookie;
  Channel channel = Channel::APP;
  PayType pay_type = PayType::ALIPAY;
};

struct Address {
  std::string id;
  std::string city_number;
  double longitude;
  double latitude;
  std::string user_name;
  std::string mobile;
  std::string address;
  std::string station_id;
};

struct Order {
  std::pair<uint64_t, uint64_t> reserve_time;
  nlohmann::json cart_data;
  nlohmann::json check_order_data;
};

class Session {
 public:
  Session() = default;
  Session(const Session &) = delete;
  Session(Session &&) = delete;
  void operator=(const Session &) = delete;

  virtual ~Session() = default;

  virtual void initUser() = 0;
  virtual void getAddresses(std::vector<Address> &) = 0;
  virtual void setAddress(const Address &) = 0;
  virtual bool cartCheckAll() = 0;
  virtual bool getCart() = 0;
  virtual bool refreshReserveTime() = 0;
  virtual void getReserveTime(std::vector<std::pair<uint64_t, uint64_t>> &) = 0;
  virtual bool checkOrder(const std::pair<uint64_t, uint64_t> &, Order &,
                          int &) = 0;
  virtual bool doOrder(Order &, int &) = 0;
  virtual int hasUnpaidOrder() = 0;

  static std::shared_ptr<Session> buildSession(SessionConfig config);
};

}  // namespace ddshop
