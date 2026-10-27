#include "DxLib.h"
#include "Reel.h"
#include <cstdlib>
#include <ctime>

// マウス左ボタンを押した瞬間だけ true を返す
bool IsNewClick(int mouseInput, int previousMouseInput)
{
    return (mouseInput & MOUSE_INPUT_LEFT) != 0 &&
        (previousMouseInput & MOUSE_INPUT_LEFT) == 0;
}

// キーが押された瞬間だけ true を返す
bool IsNewKeyPress(int keyCode, bool& previousKeyState)
{
    bool pressed = CheckHitKey(keyCode) != 0;
    bool newPress = pressed && !previousKeyState;
    previousKeyState = pressed;
    return newPress;
}
//ブドウ確率
static const int GRAPE_CHANCE_DENOMINATOR = 9;
//リプレイ確率
static const int REPLAY_CHANCE_DENOMINATOR = 6;
//チェリー確率
static const int CHERRY_CHANCE_DENOMINATOR = 25;
//ベル確率
static const int BELL_CHANCE_DENOMINATOR = 35;

int DrawRoleLottery()
{
    // 役が重なった場合は、上から先に成立した役を使う。
    if (rand() % GRAPE_CHANCE_DENOMINATOR == 0) return GRAPE;
    if (rand() % REPLAY_CHANCE_DENOMINATOR == 0) return REPLAY;
    if (rand() % CHERRY_CHANCE_DENOMINATOR == 0) return CHERRY;
    if (rand() % BELL_CHANCE_DENOMINATOR == 0) return BELL;
    return 0;
}

int DrawBonusLottery()
{
    if (rand() %  50 == 0)
    {
        return rand() % 2 + 1; // 1=BIG、2=REGULAR
    }

    return 0; // ボーナスなし
}

// ボタンを描画する
void DrawButton(int x1, int y1, int x2, int y2, const char* label)
{
    DrawBox(x1, y1, x2, y2, GetColor(80, 80, 80), TRUE);
    DrawBox(x1, y1, x2, y2, GetColor(220, 220, 220), FALSE);
    DrawString(x1 + 10, y1 + 10, label, GetColor(255, 255, 255));
}

// マウスが指定範囲内にあるか調べる
bool IsMouseIn(int mouseX, int mouseY,
    int x1, int y1, int x2, int y2)
{
    return mouseX >= x1 && mouseX <= x2 &&
        mouseY >= y1 && mouseY <= y2;
}

void ShowAtariLamp(bool& atariLamp, int& atariLampEndTime,
    int soundHandle, int duration)
{
    if (!atariLamp)
    {
        PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
    }

    atariLamp = true;
    atariLampEndTime = GetNowCount() + duration;
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

    int hitSoundHandle = LoadSoundMem("Sound/gako.mp3");
    int startSoundHandle = LoadSoundMem("Sound/reba.mp3");
    int buttonSoundHandle = LoadSoundMem("Sound/btn.mp3");
    int maxBetSoundHandle = LoadSoundMem("Sound/st.mp3");
    int bigWinSoundHandle = LoadSoundMem("Sound/i'm.mp3");

    if (hitSoundHandle == -1 ||
        startSoundHandle == -1 ||
        buttonSoundHandle == -1 ||
        maxBetSoundHandle == -1 ||
        bigWinSoundHandle == -1)
    {
        if (hitSoundHandle != -1) DeleteSoundMem(hitSoundHandle);
        if (startSoundHandle != -1) DeleteSoundMem(startSoundHandle);
        if (buttonSoundHandle != -1) DeleteSoundMem(buttonSoundHandle);
        if (maxBetSoundHandle != -1) DeleteSoundMem(maxBetSoundHandle);
        if (bigWinSoundHandle != -1) DeleteSoundMem(bigWinSoundHandle);
        DxLib_End();
        return -1;
    }

    int normalLampImage = LoadGraph("img/GoGO B.png");
    int hitLampImage = LoadGraph("img/GoGo A.png");

    if (normalLampImage == -1 || hitLampImage == -1)
    {
        if (normalLampImage != -1) DeleteGraph(normalLampImage);
        if (hitLampImage != -1) DeleteGraph(hitLampImage);
        DeleteSoundMem(hitSoundHandle);
        DeleteSoundMem(startSoundHandle);
        DeleteSoundMem(buttonSoundHandle);
        DeleteSoundMem(maxBetSoundHandle);
        DeleteSoundMem(bigWinSoundHandle);
        DxLib_End();
        return -1;
    }

    SetDrawScreen(DX_SCREEN_BACK);
    srand(static_cast<unsigned int>(time(nullptr)));

    Reel reel;

    if (!reel.LoadImages())
    {
        DeleteGraph(normalLampImage);
        DeleteGraph(hitLampImage);
        DeleteSoundMem(hitSoundHandle);
        DeleteSoundMem(startSoundHandle);
        DeleteSoundMem(buttonSoundHandle);
        DeleteSoundMem(maxBetSoundHandle);
        DeleteSoundMem(bigWinSoundHandle);
        DxLib_End();
        return -1;
    }

    // F1キーの連続入力を防ぐため、前回のキー状態を保存する
    bool previousF1 = false;

    // データカウンターの値
    int totalGames = 0;
    int bigCount = 0;
    int regularCount = 0;

    int medals = 50;
    int selectedBet = 0;
    int previousMouseInput = 0;
    bool previousSpace = false;
    bool previousEnter = false;
    bool previousM = false;
    bool previousLeftArrow = false;
    bool previousDownArrow = false;
    bool previousRightArrow = false;

    // 画面に表示するメダル枚数
    int displayedMedals = medals;

    // ゲームの進行状態


    // 停止後に結果を判定したか
    bool resultChecked = true;
    // リプレイで無料ゲームを行うか
    bool freeGameAvailable = false;


    bool atariLamp = false;
   
    int atariLampEndTime = 0;
    // ランプの点灯時間（ミリ秒）
    const int ATARI_LAMP_DURATION = 1000;
    // 次のゲームで開始するボーナス
    int queuedBonusType = 0;
    int pendingBonusType = 0;
    // ボーナス中の状態：0=通常、1=BIG、2=REGULAR
    int activeBonusType = 0;
    // ボーナス中に払い出したメダル枚数
    int bonusPaid = 0;

    while (ProcessMessage() == 0 &&
        CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        int mouseInput = GetMouseInput();
        int mouseX, mouseY;
        GetMousePoint(&mouseX, &mouseY);

        bool clicked = IsNewClick(mouseInput, previousMouseInput);
        previousMouseInput = mouseInput;

        bool spacePressed = IsNewKeyPress(KEY_INPUT_SPACE, previousSpace);
        bool enterPressed = IsNewKeyPress(KEY_INPUT_RETURN, previousEnter);
        bool mPressed = IsNewKeyPress(KEY_INPUT_M, previousM);
        bool leftArrowPressed = IsNewKeyPress(KEY_INPUT_LEFT, previousLeftArrow);
        bool downArrowPressed = IsNewKeyPress(KEY_INPUT_DOWN, previousDownArrow);
        bool rightArrowPressed = IsNewKeyPress(KEY_INPUT_RIGHT, previousRightArrow);

        // F1キーを押した瞬間にメダルを50枚追加する
        bool f1Pressed = CheckHitKey(KEY_INPUT_F1) != 0;

        if (f1Pressed && !previousF1)
        {
            medals += 50;
        }

        previousF1 = f1Pressed;

        // 表示メダルを実際の枚数へ少しずつ近づける
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

        // ベットボタンから掛け枚数を選ぶ
        if (!reel.IsSpinning())
        {
            if (((clicked && IsMouseIn(mouseX, mouseY, 200, 470, 330, 510)) ||
                (spacePressed && selectedBet == 0 && !freeGameAvailable)) &&
                activeBonusType == 0)
            {
                selectedBet = 3;
                PlaySoundMem(maxBetSoundHandle, DX_PLAYTYPE_BACK);
            }
            else if (((clicked && IsMouseIn(mouseX, mouseY, 340, 470, 440, 510)) ||
                mPressed) && activeBonusType == 0)
            {
                selectedBet = 1;
                PlaySoundMem(buttonSoundHandle, DX_PLAYTYPE_BACK);
            }
            else if ((clicked && IsMouseIn(mouseX, mouseY, 450, 470, 550, 510)) ||
                enterPressed ||
                (spacePressed &&
                    (selectedBet > 0 || freeGameAvailable || activeBonusType != 0)))
            {
                if (activeBonusType != 0)
                {
                    // ボーナス中は2枚掛け
                    if (medals >= 2)
                    {
                        medals -= 2;
                        selectedBet = 0;
                        PlaySoundMem(startSoundHandle, DX_PLAYTYPE_BACK);

                        reel.StartSpin();
                        totalGames++;
                        resultChecked = false;
                    }
                }
                else if (freeGameAvailable)
                {
                    // リプレイではメダルを使わず無料ゲームを行う
                    freeGameAvailable = false;
                    selectedBet = 0;
                    PlaySoundMem(startSoundHandle, DX_PLAYTYPE_BACK);

                    pendingBonusType = queuedBonusType;
                    queuedBonusType = 0;
                    atariLamp = false;
                    int forcedSymbol = pendingBonusType == 0 ? DrawRoleLottery() : 0;
                    reel.StartSpin(pendingBonusType, forcedSymbol);
                    totalGames++;
                    resultChecked = false;
                }
                else if (selectedBet > 0 && medals >= selectedBet)
                {
                    medals -= selectedBet;
                    PlaySoundMem(startSoundHandle, DX_PLAYTYPE_BACK);

                    pendingBonusType = queuedBonusType;
                    queuedBonusType = 0;
                    atariLamp = false;
                    int forcedSymbol = pendingBonusType == 0 ? DrawRoleLottery() : 0;
                    reel.StartSpin(pendingBonusType, forcedSymbol);
                    selectedBet = 0;
                    totalGames++;
                    resultChecked = false;
                }
            }
        }

        // 停止ボタンは左から0、1、2番リールに対応
        if (reel.IsSpinning())
        {
            if ((clicked && IsMouseIn(mouseX, mouseY, 200, 525, 300, 575)) ||
                leftArrowPressed)
            {
                reel.StopReel(0);
                PlaySoundMem(buttonSoundHandle, DX_PLAYTYPE_BACK);
            }

            if ((clicked && IsMouseIn(mouseX, mouseY, 310, 525, 410, 575)) ||
                downArrowPressed)
            {
                reel.StopReel(1);
                PlaySoundMem(buttonSoundHandle, DX_PLAYTYPE_BACK);
            }

            if ((clicked && IsMouseIn(mouseX, mouseY, 420, 525, 520, 575)) ||
                rightArrowPressed)
            {
                reel.StopReel(2);
                PlaySoundMem(buttonSoundHandle, DX_PLAYTYPE_BACK);
            }
        }

        reel.Update();

        // 全リール停止後、一度だけ結果を判定する
        if (!reel.IsSpinning() && !resultChecked)
        {
            if (pendingBonusType != 0)
            {
                // 抽選済みのボーナス図柄をそろえてボーナスを開始する
                activeBonusType = pendingBonusType;

                if (activeBonusType == 1)
                {
                    bigCount++;
                    PlaySoundMem(bigWinSoundHandle, DX_PLAYTYPE_LOOP);
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
                // ボーナス中は1ゲームごとに最大12枚を払い出す
                int payoutTarget = (activeBonusType == 1) ? 280 : 100;
                int payout = 12;
                int remaining = payoutTarget - bonusPaid;

                // 規定枚数を超えないよう最終払い出しを調整する
                if (payout > remaining)
                {
                    payout = remaining;
                }

                medals += payout;
                bonusPaid += payout;

                if (bonusPaid >= payoutTarget)
                {
                    // 規定枚数に達したらボーナスを終了する
                    if (activeBonusType == 1)
                    {
                        StopSoundMem(bigWinSoundHandle);
                    }
                    activeBonusType = 0;
                    bonusPaid = 0;
                }
            }
            else
            {
                // 通常時は5本の有効ラインを判定する
                int lineRows[5][3] =
                {
                    { 0, 0, 0 },
                    { 1, 1, 1 },
                    { 2, 2, 2 },
                    { 0, 1, 2 },
                    { 2, 1, 0 }
                };

                for (int line = 0; line < 5; line++)
                {
                    int left = reel.GetSymbolAt(0, lineRows[line][0]);
                    int center = reel.GetSymbolAt(1, lineRows[line][1]);
                    int right = reel.GetSymbolAt(2, lineRows[line][2]);

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

                queuedBonusType = DrawBonusLottery();
                if (queuedBonusType != 0)
                {
                    ShowAtariLamp(atariLamp, atariLampEndTime,
                        hitSoundHandle, ATARI_LAMP_DURATION);
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

        if (atariLamp &&
            (int)(GetNowCount() - atariLampEndTime) >= 0)
        {
            atariLamp = false;
        }

        int lampImage = (atariLamp || queuedBonusType != 0) ? hitLampImage : normalLampImage;

        DrawExtendGraph
        (
            50, 50, 192, 192,
            lampImage,
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
            620, 470, GetColor(255, 255, 255),
            "獲得枚数: %d", bonusPaid
        );

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

    DeleteSoundMem(hitSoundHandle);
    DeleteSoundMem(startSoundHandle);
    DeleteSoundMem(buttonSoundHandle);
    DeleteSoundMem(maxBetSoundHandle);
    DeleteSoundMem(bigWinSoundHandle);
    DeleteGraph(normalLampImage);
    DeleteGraph(hitLampImage);

    reel.ReleaseImages();

    DxLib_End();
    return 0; // ボーナスなし
}
