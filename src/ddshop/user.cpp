#include "session_impl.hpp"
#include "spdlog/spdlog.h"

namespace ddshop {

bool SessionImpl::initUser() {
  auto headers = base_headers_;
  auto tmp_client = httplib::Client("https://sunquan.api.ddxq.mobi");
  headers.erase("Host");
  headers.emplace("Host", "sunquan.api.ddxq.mobi");
  auto resp = tmp_client.Get("/api/v1/user/detail/", base_params_, headers);
  if (resp.error() == httplib::Error::Success) {
    nlohmann::json ret_data;
    if (!ensureBasicResp(resp->body, ret_data)) {
      spdlog::error("Failed parsing user data");
      return false;
    }
    std::string uid = ret_data["data"]["user_info"]["id"];

    spdlog::info("User uid: {}", uid);
    base_headers_.emplace("ddmc-uid", uid);
    base_params_.emplace("uid", uid);
    return true;
  } else {
    spdlog::error("Failed initializing user");
    return false;
  }
}

}  // namespace ddshop
