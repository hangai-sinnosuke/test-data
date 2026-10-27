#include "Reel.h"
#include "DxLib.h"
#include <cstdlib>

//ブドウ確率
static const int SYMBOL_WEIGHT_GRAPE = 75;
//リプレイ確率
static const int SYMBOL_WEIGHT_REPLAY = 115;
//チェリー確率
static const int SYMBOL_WEIGHT_CHERRY = 3;
//ベル確率
static const int SYMBOL_WEIGHT_BELL = 10;

static int GetSymbolWeight(int symbol)
{
    switch (symbol)
    {
    case GRAPE: return SYMBOL_WEIGHT_GRAPE;
    case REPLAY: return SYMBOL_WEIGHT_REPLAY;
    case CHERRY: return SYMBOL_WEIGHT_CHERRY;
    case BELL: return SYMBOL_WEIGHT_BELL;
    case BAR: return 1;
    case SEVEN: return 1;
    default: return 0;
    }
}

// リール上にある図柄から、重みに応じて停止位置を選ぶ
static int ChooseWeightedPosition(const int* strip, int stripLength)
{
    int totalWeight = 0;

    for (int i = 0; i < stripLength; i++)
    {
        totalWeight += GetSymbolWeight(strip[i]);
    }

    if (totalWeight <= 0)
    {
        return rand() % stripLength;
    }

    int ticket = rand() % totalWeight;

    for (int i = 0; i < stripLength; i++)
    {
        int weight = GetSymbolWeight(strip[i]);

        if (ticket < weight)
        {
            return i;
        }

        ticket -= weight;
    }

    return rand() % stripLength;
}

Reel::Reel()
{
    // リール上の図柄の並び
    int initialStrips[3][STRIP_LENGTH] =
    {
        { GRAPE, REPLAY, CHERRY, BAR, SEVEN, BELL }, // 左
        { GRAPE, REPLAY, CHERRY, BAR, SEVEN, BELL }, // 中央
        { GRAPE, REPLAY, BAR, GRAPE, SEVEN, BELL }  // 右：チェリーなし
    };

    for (int reel = 0; reel < 3; reel++)
    {
        for (int i = 0; i < STRIP_LENGTH; i++)
        {
            symbolStrips[reel][i] = initialStrips[reel][i];
        }
    }

    for (int i = 0; i < 7; i++)
    {
        imageHandles[i] = -1;
    }
}

bool Reel::LoadImages()
{
    const char* fileNames[7] =
    {
        "",
        "img/budou.png", // ブドウ
        "img/tuno.png",  // リプレイ
        "img/tyeri.png", // チェリー
        "img/ber.png",   // BAR
        "img/nana.png",  // 7
        "img/beru.png"   // ベル
    };

    for (int reel = 0; reel < 3; reel++)
    {
        positions[reel] = 0;
        targetPositions[reel] = 0;
        spinning[reel] = false;
        frameCounters[reel] = 0;
    }

    for (int symbol = 1; symbol <= 6; symbol++)
    {
        imageHandles[symbol] = LoadGraph(fileNames[symbol]);

        if (imageHandles[symbol] == -1)
        {
            ReleaseImages();
            return false;
        }
    }

    return true;
}

void Reel::ReleaseImages()
{
    for (int symbol = 1; symbol <= 6; symbol++)
    {
        if (imageHandles[symbol] != -1)
        {
            DeleteGraph(imageHandles[symbol]);
            imageHandles[symbol] = -1;
        }
    }
}

void Reel::StartSpin(int bonusType)
{
    if (bonusType == 1)
    {
        // BIGは中央段に7・7・7
        targetPositions[0] = 4;
        targetPositions[1] = 4;
        targetPositions[2] = 4;
    }
    else if (bonusType == 2)
    {
        // REGULARは7・7・BAR
        targetPositions[0] = 4;
        targetPositions[1] = 4;
        targetPositions[2] = 2;
    }
    else
    {
        // 通常時は、7やBARが有効ラインにそろわない位置を選ぶ
        int lineRows[5][3] =
        {
            { 0, 0, 0 },
            { 1, 1, 1 },
            { 2, 2, 2 },
            { 0, 1, 2 },
            { 2, 1, 0 }
        };

        bool forbidden;

        do
        {
            for (int reel = 0; reel < 3; reel++)
            {
                // 図柄ごとの設定重みに応じて停止位置を抽選
                targetPositions[reel] =
                    ChooseWeightedPosition(symbolStrips[reel], STRIP_LENGTH);
            }

            forbidden = false;
            int winningSymbol = 0;

            for (int line = 0; line < 5; line++)
            {
                int symbolsOnLine[3];

                for (int reel = 0; reel < 3; reel++)
                {
                    int stripIndex =
                        (targetPositions[reel] +
                            lineRows[line][reel] +
                            STRIP_LENGTH - 1) % STRIP_LENGTH;

                    symbolsOnLine[reel] = symbolStrips[reel][stripIndex];
                }

                // 通常時にBARや7がそろう組み合わせを除外
                if ((symbolsOnLine[0] == SEVEN &&
                    symbolsOnLine[1] == SEVEN &&
                    symbolsOnLine[2] == SEVEN) ||
                    (symbolsOnLine[0] == SEVEN &&
                        symbolsOnLine[1] == SEVEN &&
                        symbolsOnLine[2] == BAR) ||
                    (symbolsOnLine[0] == BAR &&
                        symbolsOnLine[1] == BAR &&
                        symbolsOnLine[2] == BAR))
                {
                    forbidden = true;
                }

                // 異なる種類の当たり図柄が別ラインに同時成立しないようにする
                int lineWinningSymbol = 0;
                if (symbolsOnLine[0] == GRAPE &&
                    symbolsOnLine[1] == GRAPE &&
                    symbolsOnLine[2] == GRAPE)
                {
                    lineWinningSymbol = GRAPE;
                }
                else if (symbolsOnLine[0] == BELL &&
                    symbolsOnLine[1] == BELL &&
                    symbolsOnLine[2] == BELL)
                {
                    lineWinningSymbol = BELL;
                }
                else if (symbolsOnLine[0] == REPLAY &&
                    symbolsOnLine[1] == REPLAY &&
                    symbolsOnLine[2] == REPLAY)
                {
                    lineWinningSymbol = REPLAY;
                }

                if (lineWinningSymbol != 0)
                {
                    if (winningSymbol != 0 &&
                        winningSymbol != lineWinningSymbol)
                    {
                        forbidden = true;
                    }
                    else
                    {
                        winningSymbol = lineWinningSymbol;
                    }
                }
            }
        } while (forbidden);
    }

    for (int reel = 0; reel < 3; reel++)
    {
        spinning[reel] = true;
        frameCounters[reel] = 0;
    }
}

void Reel::Update()
{
    for (int reel = 0; reel < 3; reel++)
    {
        if (!spinning[reel])
        {
            continue;
        }

        // 数フレームごとに図柄を1つ進めて回転して見せる
        frameCounters[reel]++;
        if (frameCounters[reel] >= 7)
        {
            frameCounters[reel] = 0;
            positions[reel] = (positions[reel] + 1) % STRIP_LENGTH;
        }
    }
}

void Reel::StopReel(int reel)
{
    if (reel < 0 || reel >= 3 || !spinning[reel])
    {
        return;
    }

    // 先に決めておいた位置で停止
    positions[reel] = targetPositions[reel];
    spinning[reel] = false;
}

bool Reel::IsSpinning() const
{
    return spinning[0] || spinning[1] || spinning[2];
}

void Reel::Draw() const
{
    DrawBox(200, 150, 600, 450, GetColor(30, 30, 30), TRUE);

    for (int reel = 0; reel < 3; reel++)
    {
        for (int row = 0; row < 3; row++)
        {
            int x = 205 + reel * 130;
            int y = 160 + row * 95;

            // 中段がpositionsの図柄になるように上下を選ぶ
            int stripIndex =
                (positions[reel] + row + STRIP_LENGTH - 1) % STRIP_LENGTH;
            int symbol = symbolStrips[reel][stripIndex];

            DrawExtendGraph(
                x, y, x + 125, y + 90,
                imageHandles[symbol],
                TRUE
            );
        }
    }
}

int Reel::GetCenterSymbol(int reel) const
{
    if (reel < 0 || reel >= 3)
    {
        return 0;
    }

    // positionsは中央段の図柄位置
    return symbolStrips[reel][positions[reel]];
}

int Reel::GetSymbolAt(int reel, int row) const
{
    if (reel < 0 || reel >= 3 || row < 0 || row >= 3)
    {
        return 0;
    }

    int stripIndex =
        (positions[reel] + row + STRIP_LENGTH - 1) % STRIP_LENGTH;

    return symbolStrips[reel][stripIndex];
}