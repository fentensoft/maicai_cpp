#pragma once

#include <memory>

namespace notification {

class BarkNotifier {
 public:
  virtual bool notify(const std::string &) = 0;

  static std::shared_ptr<BarkNotifier> makeBarkNotifier(const std::string &);
};

}  // namespace notification
