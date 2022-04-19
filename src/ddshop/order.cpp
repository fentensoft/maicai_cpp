#include "session_impl.hpp"
#include "spdlog/spdlog.h"
#include "uuid.h"

namespace ddshop {

bool SessionImpl::checkOrder(const std::pair<uint64_t, uint64_t> &reserve_time,
                             Order &order, int &code) {
  nlohmann::json package;

  {
    std::lock_guard<std::mutex> lck(cart_mutex_);
    if (!cart_data_.is_object() || !cart_data_.contains("products")) {
      code = -1;
      return false;
    }
    package.emplace_back(cart_data_);
    order.cart_data = cart_data_;
  }
  package.back()["reserved_time"]["reserved_time_start"] = reserve_time.first;
  package.back()["reserved_time"]["reserved_time_end"] = reserve_time.second;
  order.reserve_time = reserve_time;

  auto params = base_params_;
  params.emplace("packages", package.dump());
  params.emplace("user_ticket_id", "default");
  params.emplace("freight_ticket_id", "default");
  params.emplace("is_use_point", "0");
  params.emplace("is_use_balance", "0");
  params.emplace("is_buy_vip", "0");
  params.emplace("coupons_id", "");
  params.emplace("is_buy_coupons", "0");
  params.emplace("check_order_type", "0");
  params.emplace("is_support_merge_payment", "0");
  params.emplace("showData", "true");
  params.emplace("showMsg", "false");

  spdlog::info("Checking order");
  auto resp = client_.Post("/order/checkOrder", base_headers_, params);
  if (resp.error() == httplib::Error::Success) {
    nlohmann::json ret_json;
    if (!ensureBasicResp(resp->body, ret_json)) {
      spdlog::error("Failed to parse check order data");
      code = -1;
      return false;
    }
    code = ret_json["code"];
    if (!ret_json["success"]) {
      // TODO
      spdlog::warn("Check order return failed");
      return false;
    }
    nlohmann::json out;
    out["price"] = ret_json["data"]["order"]["total_money"];
    out["freight_discount_money"] =
        ret_json["data"]["order"]["freight_discount_money"];
    out["freight_money"] = ret_json["data"]["order"]["freight_money"];
    out["order_freight"] = ret_json["data"]["order"]["freights"][0]["freight"]
                                   ["freight_real_money"];
    out["user_ticket_id"] = ret_json["data"]["order"]["default_coupon"]["_id"];
    out["reserved_time_start"] = reserve_time.first;
    out["reserved_time_end"] = reserve_time.second;
    out["parent_order_sign"] = order.cart_data["parent_order_sign"];
    out["address_id"] = base_params_.find("address_id")->second;
    out["pay_type"] = static_cast<uint8_t>(config_.pay_type);
    out["product_type"] = 1;
    out["form_id"];
    out["receipt_without_sku"];
    out["vip_money"] = "";
    out["vip_buy_user_ticket_id"] = "";
    out["coupons_money"] = "";
    out["coupons_id"] = "";

    auto uuid_str = uuid::v4::UUID::New().String();
    uuid_str.erase(std::remove(uuid_str.begin(), uuid_str.end(), '-'),
                   uuid_str.end());
    out["form_id"] = uuid_str;

    order.check_order_data = out;

    order.cart_data["reserved_time_start"] = reserve_time.first;
    order.cart_data["reserved_time_end"] = reserve_time.second;
    order.cart_data["eta_trace_id"] = "";
    order.cart_data["soon_arrival"] = "";
    order.cart_data["first_selected_big_time"] = 0;
    order.cart_data["receipt_without_sku"] = 0;

    return true;
  } else {
    spdlog::warn("Failed check order");
    code = -1;
    return false;
  }
}

bool SessionImpl::doOrder(Order &order, int &code) {
  if (!order.check_order_data.is_object() ||
      !order.check_order_data.contains("price")) {
    code = -1;
    return false;
  }

  nlohmann::json req;
  req["payment_order"] = order.check_order_data;
  req["packages"].emplace_back(order.cart_data);

  auto params = base_params_;
  params.emplace("package_order", req.dump());
  params.emplace("showData", "true");
  params.emplace("showMsg", "false");
  params.emplace("ab_config", R"({"key_onion":"C"})");

  spdlog::info("Submitting order");
  auto resp = client_.Post("/order/addNewOrder", base_headers_, params);
  if (resp.error() == httplib::Error::Success) {
    nlohmann::json ret_json;
    if (!ensureBasicResp(resp->body, ret_json)) {
      spdlog::error("Failed to parse add new order data");
      code = -1;
      return false;
    }
    spdlog::debug("Submit order resp {}", resp->body);
    code = ret_json["code"];
    if (ret_json["success"]) {
      spdlog::info("Submit order success");
      {
        std::lock_guard<std::mutex> lck(cart_mutex_);
        cart_data_.clear();
        reserve_time_.clear();
      }
      return true;
    } else {
      // TODO
      spdlog::warn("Submit order failed, code {}, msg {}",
                   ret_json["code"].get<int>(), ret_json["msg"]);
      return false;
    }
  } else {
    // TODO
    spdlog::error("Failed to submit order");
    code = -1;
    return false;
  }
}

bool SessionImpl::hasUnpaidOrder() {
  if (base_params_.find("uid") == base_params_.end()) {
    spdlog::debug("Not login");
    return false;
  }
  spdlog::info("Fetching unpaid order list");
  auto resp = client_.Post("/order/notPayList", base_headers_, base_params_);
  if (resp.error() == httplib::Error::Success) {
    nlohmann::json ret_json;
    if (!ensureBasicResp(resp->body, ret_json)) {
      spdlog::error("Failed parsing no pay list data");
      return false;
    }
    if (!ret_json["success"]) {
      spdlog::error("Fetch no pay list data return failed");
      return false;
    }
    return !ret_json["data"]["order_list"].empty();
  } else {
    spdlog::error("Failed fetching no pay list data");
    return false;
  }
}

}  // namespace ddshop
