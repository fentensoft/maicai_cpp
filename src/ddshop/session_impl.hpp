#pragma once
#include <mutex>

#include "ddshop/session.hpp"
#include "httplib.h"

namespace ddshop {

class SessionImpl : public Session {
 public:
  explicit SessionImpl(SessionConfig config);
  void initUser() override;
  void getAddresses(std::vector<Address> &result) override;
  void setAddress(const Address &addr) override;
  bool cartCheckAll() override;
  bool getCart() override;
  bool refreshReserveTime() override;
  void getReserveTime(std::vector<std::pair<uint64_t, uint64_t>> &out) override;
  bool checkOrder(const std::pair<uint64_t, uint64_t> &reserve_time,
                  Order &order, int &code) override;
  bool doOrder(Order &order, int &code) override;
  bool hasUnpaidOrder() override;

 private:
  const std::string API_VERSION = "9.49.2";
  const std::string APP_VERSION = "2.82.0";

  SessionConfig config_;
  httplib::Client client_;
  httplib::Headers base_headers_;
  httplib::Params base_params_;

  std::mutex cart_mutex_;
  nlohmann::json cart_data_;
  std::vector<std::pair<uint64_t, uint64_t>> reserve_time_;

  bool ensureBasicResp(const std::string &, nlohmann::json &);
};

}  // namespace ddshop
