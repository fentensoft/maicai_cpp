#include "session_impl.hpp"
#include "spdlog/spdlog.h"

namespace ddshop {

bool SessionImpl::cartCheckAll() {
  if (base_params_.find("address_id") == base_params_.end()) {
    spdlog::debug("Please set address first!");
    return false;
  }

  auto params = base_params_;
  params.emplace("is_check", "1");
  auto resp = client_.Get("/cart/allCheck", params, base_headers_);

  if (resp.error() == httplib::Error::Success) {
    spdlog::debug("Check all success");
    return true;
  } else {
    spdlog::error("Failed to check all");
    return false;
  }
}

bool SessionImpl::getCart() {
  if (base_params_.find("address_id") == base_params_.end()) {
    spdlog::debug("Please set address first!");
    return false;
  }

  auto params = base_params_;
  params.emplace("is_load", "1");
  params.emplace("ab_config",
                 R"({"key_onion":"D","key_cart_discount_price":"C"})");

  spdlog::info("Getting cart products");
  auto resp = client_.Get("/cart/index", params, base_headers_);
  if (resp.error() == httplib::Error::Success) {
    auto ret_json = nlohmann::json::parse(resp->body, nullptr, false);
    if (ret_json.is_discarded()) {
      spdlog::error("Failed parsing json when getting cart");
      return false;
    }
    if (!ret_json["success"]) {
      // TODO
      spdlog::warn("Failed get cart, code {}, msg {}", ret_json["code"],
                   ret_json["msg"]);
      return false;
    }

    nlohmann::json out;
    if (ret_json["data"]["new_order_product_list"].empty()) {
      spdlog::warn("No valid product");
      {
        std::lock_guard<std::mutex> lck(cart_mutex_);
        cart_data_.swap(out);
      }
      return true;
    }
    const auto &item = ret_json["data"]["new_order_product_list"][0];
    spdlog::info("{} available items", item["products"].size());
    for (auto &it : item["products"]) {
      spdlog::info("{} x {:d} Total: {}", it["product_name"],
                   it["count"].get<uint64_t>(), it["total_price"]);
      out["products"].emplace_back(it);
      out["products"].back()["total_money"] = it["total_price"];
      out["products"].back()["total_origin_money"] = it["total_origin_price"];
    }
    spdlog::info("-------Total Price {}-------", item["total_money"]);
    out["package_type"] = item["package_type"];
    out["package_id"] = item["package_id"];
    out["total_money"] = item["total_money"];
    out["total_origin_money"] = item["total_origin_money"];
    out["goods_real_money"] = item["goods_real_money"];
    out["total_count"] = item["total_count"];
    out["cart_count"] = item["cart_count"];
    out["is_presale"] = item["is_presale"];
    out["instant_rebate_money"] = item["instant_rebate_money"];
    // out["coupon_rebate_money"] = 0;
    // out["total_rebate_money"] = 0;
    out["used_balance_money"] = item["used_balance_money"];
    out["can_used_balance_money"] = item["can_used_balance_money"];
    out["used_point_num"] = item["used_point_num"];
    out["used_point_money"] = item["used_point_money"];
    out["can_used_point_num"] = item["can_used_point_num"];
    out["can_used_point_money"] = item["can_used_point_money"];
    out["is_share_station"] = item["is_share_station"];
    out["only_today_products"] = item["only_today_products"];
    out["only_tomorrow_products"] = item["only_tomorrow_products"];
    out["front_package_text"] = item["front_package_text"];
    out["front_package_type"] = item["front_package_type"];
    out["front_package_stock_color"] = item["front_package_stock_color"];
    out["front_package_bg_color"] = item["front_package_bg_color"];

    out["parent_order_sign"] =
        ret_json["data"]["parent_order_info"]["parent_order_sign"];
    {
      std::lock_guard<std::mutex> lck(cart_mutex_);
      cart_data_.swap(out);
    }
    return true;
  } else {
    spdlog::error("Failed get cart");
    return false;
  }
}

}  // namespace ddshop