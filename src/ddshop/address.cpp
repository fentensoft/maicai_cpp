#include "session_impl.hpp"
#include "spdlog/spdlog.h"

namespace ddshop {

void SessionImpl::getAddresses(std::vector<Address> &result) {
  if (base_params_.find("uid") == base_params_.end()) {
    spdlog::debug("Please call initUser before getAddresses");
    throw std::runtime_error("No uid");
  }
  auto headers = base_headers_;
  auto params = base_params_;
  auto tmp_client = httplib::Client("https://sunquan.api.ddxq.mobi");
  headers.erase("Host");
  headers.emplace("Host", "sunquan.api.ddxq.mobi");
  params.emplace("source_type", "5");
  auto resp = tmp_client.Get("/api/v1/user/address/", params, headers);

  if (resp.error() == httplib::Error::Success) {
    nlohmann::json ret_data;
    if (!ensureBasicResp(resp->body, ret_data)) {
      spdlog::error("Failed parsing address data");
    }
    result.clear();
    if (ret_data["data"]["valid_address"].empty()) {
      spdlog::error("No valid address found");
    }
    for (auto &it : ret_data["data"]["valid_address"]) {
      Address address;
      address.id = it["id"];
      address.city_number = it["city_number"];
      address.longitude = it["location"]["location"][0];
      address.latitude = it["location"]["location"][1];
      address.user_name = it["user_name"];
      address.mobile = it["mobile"];
      address.address = it["location"]["address"].get<std::string>() + " " +
                        it["location"]["name"].get<std::string>() + " " +
                        it["addr_detail"].get<std::string>();
      address.station_id = it["station_id"];
      spdlog::debug("Parse address {} {} {} {}", address.user_name,
                    address.mobile, address.address, address.station_id);
      result.emplace_back(std::move(address));
    }

  } else {
    spdlog::error("Failed get addersses");
  }
}

void SessionImpl::setAddress(const Address &addr) {
  spdlog::info("Using address {} {}", addr.address, addr.user_name);

  base_headers_.erase("ddmc-city-number");
  base_headers_.erase("ddmc-station-id");
  base_headers_.erase("ddmc-longitude");
  base_headers_.erase("ddmc-latitude");

  base_headers_.emplace("ddmc-city-number", addr.city_number);
  base_headers_.emplace("ddmc-station-id", addr.station_id);
  base_headers_.emplace("ddmc-longitude", std::to_string(addr.longitude));
  base_headers_.emplace("ddmc-latitude", std::to_string(addr.latitude));

  base_params_.erase("address_id");
  base_params_.erase("station_id");
  base_params_.erase("city_number");
  base_params_.erase("longitude");
  base_params_.erase("latitude");

  base_params_.emplace("address_id", addr.id);
  base_params_.emplace("station_id", addr.station_id);
  base_params_.emplace("city_number", addr.city_number);
  base_params_.emplace("longitude", std::to_string(addr.longitude));
  base_params_.emplace("latitude", std::to_string(addr.latitude));
}

}  // namespace ddshop
