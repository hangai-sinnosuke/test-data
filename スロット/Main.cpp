#include "DxLib.h"
#include "Reel.h"
#include <cstdlib>
#include <ctime>

//マウスで押した瞬間だけ反応する
bool IsNewClick(int mouseInput, int previousMouseInput)
{
    return (mouseInput & MOUSE_INPUT_LEFT) != 0 &&
        (previousMouseInput & MOUSE_INPUT_LEFT) == 0;
}

// 1/255で当選し、BIGとREGULARを同じ確率で選ぶ
int DrawBonusLottery()
{
    if (rand() % 255 == 0)
    {
        return rand() % 2 + 1; // 1=BIG、2=REGULAR
    }

    return 0; // はずれ
}

const int CHERRY_COMBO_DENOMINATOR = 600; // 仮設定：1/128

//ボタンを描画する
void DrawButton(int x1, int y1, int x2, int y2, const char* label)
{
    DrawBox(x1, y1, x2, y2, GetColor(80, 80, 80), TRUE);
    DrawBox(x1, y1, x2, y2, GetColor(220, 220, 220), FALSE);
    DrawString(x1 + 10, y1 + 10, label, GetColor(255, 255, 255));
}

//マウス位置がボタンの中か調べる
bool IsMouseIn(int mouseX, int mouseY,
    int x1, int y1, int x2, int y2)
{
    return mouseX >= x1 && mouseX <= x2 &&
        mouseY >= y1 && mouseY <= y2;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    SetWindowText("DxLib Slot");
    SetGraphMode(800, 600, 32);
    ChangeWindowMode(TRUE);

    if (DxLib_Init() == -1)
    {
        return -1;
    }

    SetDrawScreen(DX_SCREEN_BACK);
    srand(static_cast<unsigned int>(time(nullptr)));

    Reel reel;

    if (!reel.LoadImages())
    {
        DxLib_End();
        return -1;
    }

    // F1キーの押しっぱなしで連続加算しないため
    bool previousF1 = false;

    //データカウンター
    int totalGames = 0;
    int bigCount = 0;
    int regularCount = 0;

    int medals = 50;
    int selectedBet = 0;
    int previousMouseInput = 0;

    // 画面表示用のメダル数
    int displayedMedals = medals;

    // チェリーで当選し、次の回転で図柄をそろえる
    int queuedBonusType = 0;

    //今回のゲームで掛けた枚数
    int currentBet = 0; 
    //停止後に結果を判定したか
    bool resultChecked = true;
    //無料ゲームが残っているか
    bool freeGameAvailable = false;

    bool gogoLamp = false;
    //当選して、図柄がそろうのを待っているボーナス
    int pendingBonusType = 0;
    //0=通常、1=BIG、2=REGULAR
    int activeBonusType = 0;
    //ボーナス中の累計払い出し
    int bonusPaid = 0;

    while (ProcessMessage() == 0 &&
        CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        int mouseInput = GetMouseInput();
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);

        bool clicked = IsNewClick(mouseInput, previousMouseInput);
        previousMouseInput = mouseInput;

        // F1を押した瞬間に50枚追加
        bool f1Pressed = CheckHitKey(KEY_INPUT_F1) != 0;

        if (f1Pressed && !previousF1)
        {
            medals += 50;
        }

        previousF1 = f1Pressed;

        // 表示上の数を、実際のメダル数へ徐々に追いつかせる
        if (displayedMedals < medals)
        {
            int step = 1 + (medals - displayedMedals) / 4;
            displayedMedals += step;

            if (displayedMedals > medals)
                displayedMedals = medals;
        }
        else if (displayedMedals > medals)
        {
            int step = 1 + (displayedMedals - medals) / 4;
            displayedMedals -= step;

            if (displayedMedals < medals)
                displayedMedals = medals;
        }

        //ベットボタンで掛け枚数を選択する
        if (clicked && !reel.IsSpinning())
        {
            if (IsMouseIn(mouseX, mouseY, 200, 470, 330, 510)
                && activeBonusType == 0)
            {
                selectedBet = 3;
            }
            else if (IsMouseIn(mouseX, mouseY, 340, 470, 440, 510)
                && activeBonusType == 0)
            {
                selectedBet = 1;
            }
            else if (IsMouseIn(mouseX, mouseY, 450, 470, 550, 510))
            {
                if (activeBonusType != 0)
                {
                    // ボーナス中は2枚掛けだけ
                    if (medals >= 2)
                    {
                        medals -= 2;
                        currentBet = 2;
                        selectedBet = 0;

                        reel.StartSpin();
                        totalGames++;
                        resultChecked = false;
                    }
                }
                else if (freeGameAvailable)
                {
                    // リプレイの無料ゲームはメダルを使わない
                    freeGameAvailable = false;
                    selectedBet = 0;
                    currentBet = 0;

                    if (queuedBonusType != 0)
                    {
                        pendingBonusType = queuedBonusType;
                        queuedBonusType = 0;
                    }
                    else
                    {
                        pendingBonusType = DrawBonusLottery();
                    }

                    gogoLamp = (pendingBonusType != 0);
                    reel.StartSpin(pendingBonusType);
                    totalGames++;
                    resultChecked = false;
                }
                else if (selectedBet > 0 && medals >= selectedBet)
                {
                    currentBet = selectedBet;
                    medals -= currentBet;

                    if (queuedBonusType != 0)
                    {
                        pendingBonusType = queuedBonusType;
                        queuedBonusType = 0;
                    }
                    else
                    {
                        pendingBonusType = DrawBonusLottery();
                    }

                    gogoLamp = (pendingBonusType != 0);
                    reel.StartSpin(pendingBonusType);
                    selectedBet = 0;
                    totalGames++;
                    resultChecked = false;
                }
            }
        }

        //停止ボタン。左から順に0、1、2
        if (clicked && reel.IsSpinning())
        {
            if (IsMouseIn(mouseX, mouseY, 200, 525, 300, 575))
                reel.StopReel(0);

            if (IsMouseIn(mouseX, mouseY, 310, 525, 410, 575))
                reel.StopReel(1);

            if (IsMouseIn(mouseX, mouseY, 420, 525, 520, 575))
                reel.StopReel(2);
        }

        reel.Update();

        // 全リールが止まった後、一度だけ結果を処理する
        if (!reel.IsSpinning() && !resultChecked)
        {
            if (pendingBonusType != 0)
            {
                // BIGまたはREGULARの図柄がそろったのでボーナス開始
                activeBonusType = pendingBonusType;

                if (activeBonusType == 1)
                {
                    bigCount++;
                }
                else
                {
                    regularCount++;
                }

                pendingBonusType = 0;
                bonusPaid = 0;
                selectedBet = 0;

            }
            else if (activeBonusType != 0)
            {
                // ボーナス中は1ゲームごとに12枚を払い出す
                int payoutTarget = (activeBonusType == 1) ? 280 : 100;
                int payout = 12;
                int remaining = payoutTarget - bonusPaid;

                // 累計が目標枚数を超えないよう、最後の払い出しを調整
                if (payout > remaining)
                {
                    payout = remaining;
                }

                medals += payout;
                bonusPaid += payout;

                if (bonusPaid >= payoutTarget)
                {
                    // 規定枚数に到達したらボーナス終了
                    activeBonusType = 0;
                    bonusPaid = 0;
                    gogoLamp = false;
                }
            }
            else
            {
                // 通常時は5本のラインを判定
                int lineRows[5][3] =
                {
                    { 0, 0, 0 }, // 上段
                    { 1, 1, 1 }, // 中段
                    { 2, 2, 2 }, // 下段
                    { 0, 1, 2 }, // 左上から右下
                    { 2, 1, 0 }  // 左下から右上
                };

                bool leftCherryOnly = false;
                bool cherryCombination = false;

                for (int line = 0; line < 5; line++)
                {
                    int left = reel.GetSymbolAt(0, lineRows[line][0]);
                    int center = reel.GetSymbolAt(1, lineRows[line][1]);
                    int right = reel.GetSymbolAt(2, lineRows[line][2]);

                    // 同じラインで左にチェリー、中央にもあれば複合
                    if (left == CHERRY)
                    {
                        if (center == CHERRY)
                        {
                            cherryCombination = true;
                        }
                        else
                        {
                            leftCherryOnly = true;
                        }
                    }

                    if (left == GRAPE && center == GRAPE && right == GRAPE)
                    {
                        medals += 8;
                    }
                    else if (left == BELL && center == BELL && right == BELL)
                    {
                        medals += 10;
                    }
                    else if (left == REPLAY &&
                        center == REPLAY &&
                        right == REPLAY)
                    {
                        freeGameAvailable = true;
                    }
                }

                // 左だけのチェリーはボーナス確定
                if (leftCherryOnly)
                {
                    queuedBonusType = rand() % 2 + 1;
                    gogoLamp = true;
                }
                // 左と中央の複合チェリーは仮確率で抽選
                else if (cherryCombination &&
                    rand() % CHERRY_COMBO_DENOMINATOR == 0)
                {
                    queuedBonusType = rand() % 2 + 1;
                    gogoLamp = true;
                }
            }

            resultChecked = true;
        }

        ClearDrawScreen();

        DrawFormatString
        (
            260, 35, GetColor(255, 0, 0),
            "BIG: %d", bigCount
        );

        DrawLine(250, 65, 430, 65, GetColor(220, 220, 220));

        DrawFormatString
        (
            260, 75, GetColor(0, 100, 255),
            "REGULAR: %d", regularCount
        );

        DrawFormatString
        (
            470, 55, GetColor(0, 255, 0),
            "Games: %d", totalGames
        );

        reel.Draw();

        DrawCircle
        (
            700, 220, 25,
            gogoLamp ? GetColor(255, 0, 0) : GetColor(60, 60, 60),
            TRUE
        );

        DrawButton(200, 470, 330, 510, "MAX BET");
        DrawButton(340, 470, 440, 510, "1 BET");
        DrawButton(450, 470, 550, 510, "START");

        DrawButton(200, 525, 300, 575, "STOP 1");
        DrawButton(310, 525, 410, 575, "STOP 2");
        DrawButton(420, 525, 520, 575, "STOP 3");

        DrawFormatString
        (
            620, 500, GetColor(255, 255, 255),
            "Medal: %d", displayedMedals
        );

        DrawFormatString
        (
            620, 530, GetColor(255, 255, 255),
            "Bet: %d", selectedBet
        );

        ScreenFlip();
    }

    reel.ReleaseImages();
    DxLib_End();
    return 0;
}