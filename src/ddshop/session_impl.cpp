#include "session_impl.hpp"

#include "spdlog/spdlog.h"

namespace ddshop {

SessionImpl::SessionImpl(SessionConfig config)
    : config_(std::move(config)), client_("https://maicai.api.ddxq.mobi") {
  if (config_.cookie.empty()) {
    spdlog::error("Cookie should not be empty");
    throw std::runtime_error("Empty cookie");
  }
  spdlog::info("Initializing session with cookie {}", config_.cookie);

  base_headers_.clear();
  base_headers_.emplace("cookie", "DDXQSESSID=" + config_.cookie);
  base_headers_.emplace("Host", "maicai.api.ddxq.mobi");
  base_headers_.emplace(
      "user-agent",
      "Mozilla/5.0 (iPhone; CPU iPhone OS 11_3 like Mac OS X) "
      "AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E217 "
      "MicroMessenger/6.8.0(0x16080000) NetType/WIFI Language/en "
      "Branch/Br_trunk MiniProgramEnv/Mac");
  base_headers_.emplace("accept", "application/json, text/plain, */*");
  base_headers_.emplace("origin", "https://wx.m.ddxq.mobi");
  base_headers_.emplace("sec-fetch-site", "same-site");
  base_headers_.emplace("sec-fetch-mode", "cors");
  base_headers_.emplace("sec-fetch-dest", "empty");
  base_headers_.emplace("referer", "https://wx.m.ddxq.mobi/");
  base_headers_.emplace("accept-language",
                        "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7");
  base_headers_.emplace("ddmc-os-version", "undefined");
  base_headers_.emplace("ddmc-channel", "applet");
  base_headers_.emplace("ddmc-api-version", API_VERSION);
  base_headers_.emplace("ddmc-build-version", APP_VERSION);
  base_headers_.emplace("ddmc-app-client-id",
                        std::to_string(static_cast<uint8_t>(config_.channel)));
  base_headers_.emplace("ddmc-ip", "");

  base_params_.emplace("channel", "applet");
  base_params_.emplace("api_version", API_VERSION);
  base_params_.emplace("app_version", APP_VERSION);
  base_params_.emplace("app_client_id",
                       std::to_string(static_cast<uint8_t>(config_.channel)));
  base_params_.emplace("applet_source", "");
  base_params_.emplace("h5_source", "");
  base_params_.emplace("sharer_uid", "");
  base_params_.emplace("s_id", "");
  base_params_.emplace("openid", "");
  base_params_.emplace("device_token", "");
  base_params_.emplace("nars", "");
  base_params_.emplace("sesi", "");

  client_.set_connection_timeout(std::chrono::milliseconds(500));
  client_.set_read_timeout(std::chrono::milliseconds(2000));
  client_.set_write_timeout(std::chrono::milliseconds(2000));
}

bool SessionImpl::ensureBasicResp(const std::string &str, nlohmann::json &out) {
  out = nlohmann::json::parse(str, nullptr, false);
  if (out.is_discarded()) {
    spdlog::error("JSON parse error");
    spdlog::debug("{}", str);
    return false;
  }
  if (!out.contains("success") || !out["success"].is_boolean()) {
    spdlog::error("JSON invalid: no success field");
    spdlog::debug("{}", str);
    return false;
  }
  if (!out["success"] && (!out.contains("code") || !out["code"].is_number() ||
                          !out.contains("msg") || !out["msg"].is_string())) {
    spdlog::error("JSON not success, but invalid code or msg");
    spdlog::debug("{}", str);
    return false;
  }
  if (out["success"] && !out.contains("data")) {
    spdlog::error("JSON success, but no data");
    spdlog::debug("{}", str);
    return false;
  }
  return true;
}

std::shared_ptr<Session> Session::buildSession(SessionConfig config) {
  return std::make_shared<SessionImpl>(std::move(config));
}

}  // namespace ddshop
