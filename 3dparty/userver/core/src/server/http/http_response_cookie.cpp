#include <userver/server/http/http_response_cookie.hpp>

#include <array>

#include <fmt/compile.h>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const std::string kTimeFormat = "%a, %d %b %Y %H:%M:%S %Z";
}

namespace server::http {

/// CookieData class

class Cookie::CookieData final {
 public:
  CookieData(std::string&& name, std::string&& value);
  CookieData(const CookieData&) = default;
  ~CookieData() = default;

  [[nodiscard]] const std::string& Name() const;
  [[nodiscard]] const std::string& Value() const;

  [[nodiscard]] bool IsSecure() const;
  void SetSecure();

  [[nodiscard]] std::chrono::system_clock::time_point Expires() const;
  void SetExpires(std::chrono::system_clock::time_point value);

  [[nodiscard]] bool IsPermanent() const;
  void SetPermanent();

  [[nodiscard]] bool IsHttpOnly() const;
  void SetHttpOnly();

  [[nodiscard]] const std::string& Path() const;
  void SetPath(std::string&& value);

  [[nodiscard]] const std::string& Domain() const;
  void SetDomain(std::string&& value);

  [[nodiscard]] std::chrono::seconds MaxAge() const;
  void SetMaxAge(std::chrono::seconds value);

  void AppendToString(std::string& os) const;

 private:
  void ValidateName() const;
  void ValidateValue() const;

  std::string name_;
  std::string value_;
  std::string path_;
  std::string domain_;

  bool secure_{false};
  bool http_only_{false};
  std::chrono::system_clock::time_point expires_{};
  std::chrono::seconds max_age_{0};
};

Cookie::CookieData::CookieData(std::string&& name, std::string&& value)
    : name_(std::move(name)), value_(std::move(value)) {
  ValidateName();
  ValidateValue();
}

const std::string& Cookie::CookieData::Name() const { return name_; }
const std::string& Cookie::CookieData::Value() const { return value_; }

bool Cookie::CookieData::IsSecure() const { return secure_; }
void Cookie::CookieData::SetSecure() { secure_ = true; }

std::chrono::system_clock::time_point Cookie::CookieData::Expires() const {
  return expires_;
}
void Cookie::CookieData::SetExpires(
    std::chrono::system_clock::time_point value) {
  expires_ = value;
}

bool Cookie::CookieData::IsPermanent() const {
  return expires_ == std::chrono::system_clock::time_point::max();
}
void Cookie::CookieData::SetPermanent() {
  expires_ = std::chrono::system_clock::time_point::max();
}

bool Cookie::CookieData::IsHttpOnly() const { return http_only_; }
void Cookie::CookieData::SetHttpOnly() { http_only_ = true; }

const std::string& Cookie::CookieData::Path() const { return path_; }
void Cookie::CookieData::SetPath(std::string&& value) {
  path_ = std::move(value);
}

const std::string& Cookie::CookieData::Domain() const { return domain_; }
void Cookie::CookieData::SetDomain(std::string&& value) {
  domain_ = std::move(value);
}

std::chrono::seconds Cookie::CookieData::MaxAge() const { return max_age_; }
void Cookie::CookieData::SetMaxAge(std::chrono::seconds value) {
  max_age_ = value;
}

void Cookie::CookieData::AppendToString(std::string& os) const {
  os.append(name_);
  os.append("=");
  os.append(value_);

  if (!domain_.empty()) {
    os.append("; Domain=");
    os.append(domain_);
  }
  if (!path_.empty()) {
    os.append("; Path=");
    os.append(path_);
  }
  if (expires_ > std::chrono::system_clock::time_point{}) {
    os.append("; Expires=");
    os.append(utils::datetime::Timestring(expires_, "GMT", kTimeFormat));
  }
  if (max_age_ > std::chrono::seconds{0}) {
    os.append("; Max-Age=");
    fmt::format_to(std::back_inserter(os), FMT_COMPILE("{}"), max_age_.count());
  }
  if (secure_) {
    os.append("; Secure");
  }
  if (http_only_) {
    os.append("; HttpOnly");
  }
}

void Cookie::CookieData::ValidateName() const {
  static constexpr auto kBadNameChars = []() {
    std::array<bool, 256> res{};  // Zero initializes
    for (int i = 0; i < 32; i++) res[i] = true;
    for (int i = 127; i < 256; i++) res[i] = true;
    for (unsigned char c : "()<>@,;:\\\"/[]?={} \t") res[c] = true;
    return res;
  }();

  if (name_.empty()) throw std::runtime_error("Empty cookie name");

  for (char c : name_) {
    auto code = static_cast<uint8_t>(c);
    if (kBadNameChars[code]) {
      throw std::runtime_error(
          fmt::format("Invalid character in cookie name: '{}' (#{})", c, code));
    }
  }
}

void Cookie::CookieData::ValidateValue() const {
  // `cookie-value` from https://tools.ietf.org/html/rfc6265#section-4.1.1
  static constexpr auto kBadValueChars = []() {
    std::array<bool, 256> res{};  // Zero initializes
    for (int i = 0; i <= 32; i++) res[i] = true;
    for (int i = 127; i < 256; i++) res[i] = true;
    res[0x22] = true;  // `"`
    res[0x2C] = true;  // `,`
    res[0x3B] = true;  // `;`
    res[0x5C] = true;  // `\`
    return res;
  }();

  std::string_view value(value_);
  if (value.size() > 1 && value.front() == '"' && value.back() == '"')
    value = value.substr(1, value.size() - 2);

  for (char c : value) {
    auto code = static_cast<uint8_t>(c);
    if (kBadValueChars[code]) {
      throw std::runtime_error(fmt::format(
          "Invalid character in cookie value: '{}' (#{})", c, code));
    }
  }
}

// Cookie class

Cookie::Cookie(std::string name, std::string value)
    : data_(std::make_unique<CookieData>(std::move(name), std::move(value))) {}
Cookie::Cookie(Cookie&& cookie) noexcept = default;
Cookie::~Cookie() noexcept = default;
Cookie::Cookie(const Cookie& cookie) { *this = cookie; }

Cookie& Cookie::operator=(Cookie&&) noexcept = default;
Cookie& Cookie::operator=(const Cookie& cookie) {
  if (this == &cookie) return *this;

  data_ = std::make_unique<CookieData>(*cookie.data_);
  return *this;
}

const std::string& Cookie::Name() const { return data_->Name(); }
const std::string& Cookie::Value() const { return data_->Value(); }

bool Cookie::IsSecure() const { return data_->IsSecure(); }
Cookie& Cookie::SetSecure() {
  data_->SetSecure();
  return *this;
}

std::chrono::system_clock::time_point Cookie::Expires() const {
  return data_->Expires();
}
Cookie& Cookie::SetExpires(std::chrono::system_clock::time_point value) {
  data_->SetExpires(value);
  return *this;
}

bool Cookie::IsPermanent() const { return data_->IsPermanent(); }
Cookie& Cookie::SetPermanent() {
  data_->SetPermanent();
  return *this;
}

bool Cookie::IsHttpOnly() const { return data_->IsHttpOnly(); }
Cookie& Cookie::SetHttpOnly() {
  data_->SetHttpOnly();
  return *this;
}

const std::string& Cookie::Path() const { return data_->Path(); }
Cookie& Cookie::SetPath(std::string value) {
  data_->SetPath(std::move(value));
  return *this;
}

const std::string& Cookie::Domain() const { return data_->Domain(); }
Cookie& Cookie::SetDomain(std::string value) {
  data_->SetDomain(std::move(value));
  return *this;
}

std::chrono::seconds Cookie::MaxAge() const { return data_->MaxAge(); }
Cookie& Cookie::SetMaxAge(std::chrono::seconds value) {
  data_->SetMaxAge(value);
  return *this;
}

std::string Cookie::ToString() const {
  std::string os;
  data_->AppendToString(os);
  return os;
}

void Cookie::AppendToString(std::string& os) const {
  data_->AppendToString(os);
}

}  // namespace server::http

USERVER_NAMESPACE_END
