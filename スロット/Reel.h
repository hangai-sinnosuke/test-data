#pragma once

enum Symbol
{
    GRAPE = 1, // ブドウ
    REPLAY,    // ゾウのリプレイ
    CHERRY,    // チェリー
    BAR,       // BAR
    SEVEN,     // 7
    BELL       // ベル
};

class Reel
{
public:
    Reel();

    bool LoadImages();
    void ReleaseImages();

    // bonusType: 0=通常、1=BIG、2=REGULAR
    void StartSpin(int bonusType = 0, int forcedSymbol = 0);
    void Update();
    void StopReel(int reel);

    bool IsSpinning() const;
    void Draw() const;
    int GetSymbolAt(int reel, int row) const;

private:
    static const int STRIP_LENGTH = 6;
    static const int SYMBOL_WIDTH = 125;
    static const int SYMBOL_HEIGHT = 90;

    int symbolStrips[3][STRIP_LENGTH];
    int imageHandles[7];
    int stripImageHandles[3];

    int positions[3];
    int targetPositions[3];
    float scrollOffsets[3];
    float stopTargetOffsets[3];

    bool spinning[3];
    bool stopping[3];
};
