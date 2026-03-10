#pragma once
;

class User
{
  public:
    static vector<Tile *> allowedTiles;
    static void staticCtor();
    wstring name;
    wstring sessionId;
    wstring mpPassword;

    User(const wstring &name, const wstring &sessionId);
};
