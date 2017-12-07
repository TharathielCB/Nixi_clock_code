#ifndef BTNMENU
  #define BTNMENU

  typedef struct Node{
    struct Node *prev;
    struct Node *next;
    void (*command)();
    void (*btn_left_action)();
    void (*btn_right_action)();
    void (*btn_center_action)();
    void (*btn_left_long_action)();
    void (*btn_right_long_action)();
    void (*btn_center_long_action)();
  }node;



void create_menu();

#endif
