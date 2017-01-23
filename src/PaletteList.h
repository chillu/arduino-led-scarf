/**
 * Represents a list of palettes that you can switch between
 */
class PaletteList {
  private:
    byte _curr;
    byte _num;
    CRGBPalette16 **_palettes;

  public:
    PaletteList(byte num, CRGBPalette16 **palettes): _curr(0), _num(num), _palettes(palettes) {
    }

    /**
     * Switch to the next
     */
    CRGBPalette16* next() {
      _curr = (_curr + 1) % _num;
      return _curr;
    }

    CRGBPalette16* curr()
    {
      return _palettes[_curr];
    }

    /**
     * Switch to a random pattern
     */
    void rand() {
      _curr = random(_num);
    }
};
