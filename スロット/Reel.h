#pragma once

enum Symbol
{
    //ブドウ
    GRAPE = 1,
    //ゾウのリプレイ
    REPLAY,
    //チェリー
    CHERRY,
    //BAR
    BAR,
    //7
    SEVEN,
    //ベル
    BELL
};

class Reel
{
public:
    Reel();

    bool LoadImages();
    void ReleaseImages();

    //bonusType: 0=通常、1=BIG、2=REGULAR
    void StartSpin(int bonusType = 0);

    //回転中の表示を更新する
    void Update();

    //指定したリールを停止する（0=左、1=中、2=右）
    void StopReel(int reel);

    bool IsSpinning() const;
    void Draw() const;

    // 指定リールの中央段の図柄番号を返す
    int GetCenterSymbol(int reel) const;

    // 指定リール・段の図柄番号を返す（段は0=上、1=中央、2=下）
    int GetSymbolAt(int reel, int row) const;

private:
    static const int STRIP_LENGTH = 6;

    //リールに並ぶ図柄
    int symbolStrips[3][STRIP_LENGTH];

    //各リールの現在位置と、回転開始時に決めた停止位置
    int positions[3];
    int targetPositions[3];

    bool spinning[3];
    int frameCounters[3];

    int imageHandles[7];
};