#include "Reel.h"
#include "DxLib.h"
#include <cstdlib>

Reel::Reel()
{
    //リール上の図柄の並び。後で確率に合わせて調整できる
    int initialStrips[3][STRIP_LENGTH] =
    {
        { GRAPE, REPLAY, GRAPE, BAR, SEVEN, BELL }, // 左
        { GRAPE, REPLAY, CHERRY, BAR, SEVEN, BELL }, // 中央
        { GRAPE, REPLAY, BAR, GRAPE, SEVEN, BELL }   // 右：チェリーなし
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
        "img/budou.png", //ブドウ
        "img/tuno.png",  //ゾウのリプレイ
        "img/tyeri.png", //チェリー
        "img/ber.png",   //BAR
        "img/nana.png",  //7
        "img/beru.png"   //ベル
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
        // REGULAR：7・7・BAR
        targetPositions[0] = 4;
        targetPositions[1] = 4;
        targetPositions[2] = 2;
    }
    else
    {
        // 通常時は、7やBARが有効ラインにそろわない位置を選ぶ
        int lineRows[5][3] =
        {
            { 0, 0, 0 },{ 1, 1, 1 },{ 2, 2, 2 },
            { 0, 1, 2 },{ 2, 1, 0 }
        };

        bool forbidden;

        do
        {
            for (int reel = 0; reel < 3; reel++)
            {
                targetPositions[reel] = rand() % STRIP_LENGTH;
            }

            forbidden = false;

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

        //数フレームごとに図柄を1つ進めて回転して見せる
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

    //先に決めておいた位置で停止する
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