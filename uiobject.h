class MessageBox
{
  public:
    MessageBox(const uint8_t *r) : rect(r) {  }

    void setText(const char *t);
    void draw(int line, uint8_t lineBuffer[96 * 2]);

  private:
    const uint8_t *rect;
    char text[7][16];
};

class Dialog
{
  public:
    Dialog(const uint8_t *r) : rect(r) {  }

    uint8_t process();
    void draw(int line, uint8_t lineBuffer[96 * 2]);
    void setOptions(uint8_t col, uint8_t c, const char **o);
    void setRect(const uint8_t *r) { rect = r; }

  private:
    const uint8_t *rect;
    uint8_t top;
    uint8_t where;
    uint8_t column;
    uint8_t count;
    const char **option;
};

extern MessageBox bottomMessage;
extern Dialog choice;
