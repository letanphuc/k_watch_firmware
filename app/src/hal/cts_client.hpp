#ifndef CTS_CLIENT_HPP
#define CTS_CLIENT_HPP

class CtsClient {
 public:
  static CtsClient& instance() {
    static CtsClient inst;
    return inst;
  }

  int init();

 private:
  CtsClient() = default;
};

#endif /* CTS_CLIENT_HPP */
