#include "bark_impl.hpp"

#include "httplib.h"
#include "spdlog/spdlog.h"

namespace notification {

BarkNotifierImpl::BarkNotifierImpl(std::string bark_id)
    : bark_id_(std::move(bark_id)) {
  spdlog::info("Bark notifier initialized with bark_id {}", bark_id_);
}

bool BarkNotifierImpl::notify(const std::string &msg) {
  httplib::Client client("https://api.day.app");
  httplib::Params params{{"sound", "minuet"}};
  httplib::Headers headers{{"Accept-Encoding", "gzip, deflate"}};

  auto resp = client.Get(("/" + bark_id_ + "/" + msg).c_str(), params, headers);
  return (resp.error() == httplib::Error::Success);
}

std::shared_ptr<BarkNotifier> BarkNotifier::makeBarkNotifier(
    const std::string &bark_id) {
  return std::make_shared<BarkNotifierImpl>(bark_id);
}

}  // namespace notification