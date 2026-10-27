#include "Reel.h"
#include "DxLib.h"
#include <cstdlib>
#include <cmath>

// ブドウ図柄を選ぶ重み（当選確率そのものではない）
static const int SYMBOL_WEIGHT_GRAPE = 20;
// リプレイ成立確率は Main.cpp の分母設定で決まる
// チェリー図柄を選ぶ重み
static const int SYMBOL_WEIGHT_CHERRY = 20;
// ベル図柄を選ぶ重み
static const int SYMBOL_WEIGHT_BELL = 20;

static int GetSymbolWeight(int symbol)
{
    switch (symbol)
    {
    case GRAPE: return SYMBOL_WEIGHT_GRAPE;
    case REPLAY: return 0;
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
         { GRAPE, CHERRY, REPLAY, BAR, SEVEN, BELL }, // 中央：リプレイの前をチェリーにする
         { GRAPE, REPLAY, BAR, GRAPE, SEVEN, BELL }  // 右：チェリーなし
    };

    for (int reel = 0; reel < 3; reel++)
    {
        for (int i = 0; i < STRIP_LENGTH; i++)
        {
            symbolStrips[reel][i] = initialStrips[reel][i];
        }
    }

    for (int reel = 0; reel < 3; reel++)
    {
        positions[reel] = 0;
        targetPositions[reel] = 0;
        scrollOffsets[reel] = 0.0f;
        stopTargetOffsets[reel] = 0.0f;
        spinning[reel] = false;
        stopping[reel] = false;
        stripImageHandles[reel] = -1;
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


    for (int symbol = 1; symbol <= 6; symbol++)
    {
        imageHandles[symbol] = LoadGraph(fileNames[symbol]);

        if (imageHandles[symbol] == -1)
        {
            ReleaseImages();
            return false;
        }
    }

    // 各リールの図柄を縦につないだ一枚の画像を作る
    int previousDrawScreen = GetDrawScreen();
    for (int reel = 0; reel < 3; reel++)
    {
        int stripHeight = STRIP_LENGTH * SYMBOL_HEIGHT;
        stripImageHandles[reel] = MakeScreen(SYMBOL_WIDTH, stripHeight, TRUE);
        if (stripImageHandles[reel] == -1)
        {
            SetDrawScreen(previousDrawScreen);
            ReleaseImages();
            return false;
        }

        if (SetDrawScreen(stripImageHandles[reel]) == -1)
        {
            SetDrawScreen(previousDrawScreen);
            ReleaseImages();
            return false;
        }

        ClearDrawScreen();
        for (int index = 0; index < STRIP_LENGTH; index++)
        {
            int symbol = symbolStrips[reel][index];
            DrawExtendGraph(
                0, index * SYMBOL_HEIGHT,
                SYMBOL_WIDTH, (index + 1) * SYMBOL_HEIGHT,
                imageHandles[symbol], TRUE
            );
        }
    }
    SetDrawScreen(previousDrawScreen);
    return true;
}

void Reel::ReleaseImages()
{
    for (int reel = 0; reel < 3; reel++)
    {
        if (stripImageHandles[reel] != -1)
        {
            DeleteGraph(stripImageHandles[reel]);
            stripImageHandles[reel] = -1;
        }
    }

    for (int symbol = 1; symbol <= 6; symbol++)
    {
        if (imageHandles[symbol] != -1)
        {
            DeleteGraph(imageHandles[symbol]);
            imageHandles[symbol] = -1;
        }
    }
}

void Reel::StartSpin(int bonusType, int forcedSymbol)
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
    else if (forcedSymbol != 0)
    {
        // 抽選された役を中央段に表示する。チェリーは左リールだけに表示する。
        for (int reel = 0; reel < 3; reel++)
        {
            if (forcedSymbol == CHERRY && reel > 0)
            {
                targetPositions[reel] = ChooseWeightedPosition(symbolStrips[reel], STRIP_LENGTH);
                continue;
            }

            targetPositions[reel] = 0;
            for (int index = 0; index < STRIP_LENGTH; index++)
            {
                if (symbolStrips[reel][index] == forcedSymbol)
                {
                    targetPositions[reel] = index;
                    break;
                }
            }
        }
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
                    // リプレイ成立は専用の1/N抽選だけで起こす
                    forbidden = true;
                }

                // 通常抽選では、ブドウ・ベルの揃いも作らない。
                if (lineWinningSymbol != 0)
                {
                    forbidden = true;
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
        stopping[reel] = false;
        stopTargetOffsets[reel] = scrollOffsets[reel];
    }
}

void Reel::Update()
{
    const float normalSpeed = 12.0f;

    for (int reel = 0; reel < 3; reel++)
    {
        if (!spinning[reel])
        {
            continue;
        }

        if (!stopping[reel])
        {
            // 回転中はストリップ画像を一定速度で流す
            scrollOffsets[reel] -= normalSpeed;
            continue;
        }

        // 停止位置に近づくほど速度を落とす
        float remaining = scrollOffsets[reel] - stopTargetOffsets[reel];
        float step = remaining * 0.08f;
        if (step > normalSpeed)
        {
            step = normalSpeed;
        }
        if (step < 1.0f)
        {
            step = 1.0f;
        }

        if (remaining <= step)
        {
            scrollOffsets[reel] = stopTargetOffsets[reel];
            positions[reel] = targetPositions[reel];
            spinning[reel] = false;
            stopping[reel] = false;
        }
        else
        {
            scrollOffsets[reel] -= step;
        }
    }
}

void Reel::StopReel(int reel)
{
    if (reel < 0 || reel >= 3 || !spinning[reel] || stopping[reel])
    {
        return;
    }

    const float stripHeight = static_cast<float>(STRIP_LENGTH * SYMBOL_HEIGHT);
    float currentCenter = std::fmod(scrollOffsets[reel], stripHeight);
    if (currentCenter < 0.0f)
    {
        currentCenter += stripHeight;
    }

    float targetCenter = static_cast<float>(targetPositions[reel] * SYMBOL_HEIGHT);
    float distance = currentCenter - targetCenter;
    if (distance < 0.0f)
    {
        distance += stripHeight;
    }

    // ボタンを押してから少なくとも2周して停止する
    distance += stripHeight * 2.0f;
    stopTargetOffsets[reel] = scrollOffsets[reel] - distance;
    stopping[reel] = true;
}

bool Reel::IsSpinning() const
{
    return spinning[0] || spinning[1] || spinning[2];
}

void Reel::Draw() const
{
    DrawBox(200, 150, 600, 450, GetColor(30, 30, 30), TRUE);

    const int stripHeight = STRIP_LENGTH * SYMBOL_HEIGHT;
    const int viewHeight = SYMBOL_HEIGHT * 3;

    for (int reel = 0; reel < 3; reel++)
    {
        int x = 205 + reel * 130;
        int y = 160;
        int centerPixel = static_cast<int>(scrollOffsets[reel]) % stripHeight;
        if (centerPixel < 0)
        {
            centerPixel += stripHeight;
        }
        int sourceY = (centerPixel - SYMBOL_HEIGHT + stripHeight) % stripHeight;
        int firstHeight = viewHeight;

        // ストリップ末尾を越えたら、先頭部分を続けて描く
        if (sourceY + firstHeight > stripHeight)
        {
            firstHeight = stripHeight - sourceY;
        }

        DrawRectGraph(
            x, y, 0, sourceY,
            SYMBOL_WIDTH, firstHeight,
            stripImageHandles[reel], TRUE
        );

        int remainingHeight = viewHeight - firstHeight;
        if (remainingHeight > 0)
        {
            DrawRectGraph(
                x, y + firstHeight, 0, 0,
                SYMBOL_WIDTH, remainingHeight,
                stripImageHandles[reel], TRUE
            );
        }
    }
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