#include <string>

#include "notification/bark.hpp"

namespace notification {

class BarkNotifierImpl : public BarkNotifier {
 public:
  explicit BarkNotifierImpl(std::string);

  bool notify(const std::string &) override;

 private:
  std::string bark_id_;
};

}  // namespace notification