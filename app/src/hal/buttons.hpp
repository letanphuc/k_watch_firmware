#ifndef BUTTONS_HPP
#define BUTTONS_HPP

class Buttons {
 public:
  static Buttons& instance() {
    static Buttons inst;
    return inst;
  }

  int init();

 private:
  Buttons() = default;
};

#endif /* BUTTONS_HPP */
