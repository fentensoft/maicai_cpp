#include "session_impl.hpp"
#include "spdlog/spdlog.h"

namespace ddshop {

bool SessionImpl::refreshReserveTime() {
  nlohmann::json products;
  {
    std::lock_guard<std::mutex> lck(cart_mutex_);
    if (!cart_data_.is_object() || !cart_data_.contains("products")) {
      return false;
    }
    products.emplace_back(cart_data_["products"]);
  }
  auto params = base_params_;

  params.emplace("products", products.dump());
  params.emplace("group_config_id", "");
  params.emplace("isBridge", "false");

  auto resp = client_.Post("/order/getMultiReserveTime", base_headers_, params);
  if (resp.error() == httplib::Error::Success) {
    nlohmann::json ret_json;
    if (!ensureBasicResp(resp->body, ret_json)) {
      spdlog::error("Failed parse getReserveTime data");
      spdlog::debug(resp->body);
      return false;
    }
    if (ret_json["data"].empty() || ret_json["data"][0]["time"].empty()) {
      spdlog::warn("No available reserve time");
      {
        std::lock_guard<std::mutex> lck(cart_mutex_);
        reserve_time_.clear();
      }
      return false;
    }
    std::vector<std::pair<uint64_t, uint64_t>> out;
    for (auto &it : ret_json["data"][0]["time"][0]["times"]) {
      if (it["disableType"] != 0) {
        continue;
      }
      spdlog::debug("Found reserve time {} to {} {}",
                    it["start_timestamp"].get<uint64_t>(),
                    it["end_timestamp"].get<uint64_t>(), it["select_msg"]);
      out.emplace_back(std::make_pair(it["start_timestamp"].get<uint64_t>(),
                                      it["end_timestamp"].get<uint64_t>()));
    }
    spdlog::info("Found {} available reserve time", out.size());
    {
      std::lock_guard<std::mutex> lck(cart_mutex_);
      if (!cart_data_.is_object() || !cart_data_.contains("products")) {
        return false;
      }
      reserve_time_.swap(out);
    }
    return true;
  } else {
    spdlog::warn("Failed get reserve time");
    return false;
  }
}

void SessionImpl::getReserveTime(
    std::vector<std::pair<uint64_t, uint64_t>> &out) {
  out.clear();
  std::lock_guard<std::mutex> lck(cart_mutex_);
  out.assign(reserve_time_.begin(), reserve_time_.end());
}

}  // namespace ddshop
