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
bool IsNewKeyPress(int keyCode, bool& previousKeyState)
{
    bool pressed = CheckHitKey(keyCode) != 0;
    bool newPress = pressed && !previousKeyState;
    previousKeyState = pressed;
    return newPress;
}

int DrawBonusLottery()
{
    if (rand() %  5== 0)
    {
        return rand() % 2 + 1; // 1=BIG、2=REGULAR
    }

    return 0; // はずれ
}

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

    if (hitSoundHandle == -1 ||
        startSoundHandle == -1 ||
        buttonSoundHandle == -1)
    {
        if (hitSoundHandle != -1) DeleteSoundMem(hitSoundHandle);
        if (startSoundHandle != -1) DeleteSoundMem(startSoundHandle);
        if (buttonSoundHandle != -1) DeleteSoundMem(buttonSoundHandle);
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
    bool previousSpace = false;
    bool previousEnter = false;
    bool previousM = false;
    bool previousLeftArrow = false;
    bool previousDownArrow = false;
    bool previousRightArrow = false;

    // 画面表示用のメダル数
    int displayedMedals = medals;

    // チェリーで当選し、次の回転で図柄をそろえる
    //今回のゲームで掛けた枚数
    int currentBet = 0; 
    //停止後に結果を判定したか
    bool resultChecked = true;
    //無料ゲームが残っているか
    bool freeGameAvailable = false;
    //ランプが光ったかの判別
    bool gogoLamp = false;

    bool atariLamp = false;
   
    int atariLampEndTime = 0;
    //表示時間：ミリ秒
    const int ATARI_LAMP_DURATION = 1000;
    //当選して、図柄がそろうのを待っているボーナス
    int queuedBonusType = 0;
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

        bool spacePressed = IsNewKeyPress(KEY_INPUT_SPACE, previousSpace);
        bool enterPressed = IsNewKeyPress(KEY_INPUT_RETURN, previousEnter);
        bool mPressed = IsNewKeyPress(KEY_INPUT_M, previousM);
        bool leftArrowPressed = IsNewKeyPress(KEY_INPUT_LEFT, previousLeftArrow);
        bool downArrowPressed = IsNewKeyPress(KEY_INPUT_DOWN, previousDownArrow);
        bool rightArrowPressed = IsNewKeyPress(KEY_INPUT_RIGHT, previousRightArrow);

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
        if (!reel.IsSpinning())
        {
            if (((clicked && IsMouseIn(mouseX, mouseY, 200, 470, 330, 510)) ||
                (spacePressed && selectedBet == 0 && !freeGameAvailable)) &&
                activeBonusType == 0)
            {
                selectedBet = 3;
                PlaySoundMem(buttonSoundHandle, DX_PLAYTYPE_BACK);
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
                    // ボーナス中は2枚掛けだけ
                    if (medals >= 2)
                    {
                        medals -= 2;
                        currentBet = 2;
                        selectedBet = 0;
                        PlaySoundMem(startSoundHandle, DX_PLAYTYPE_BACK);

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
                    PlaySoundMem(startSoundHandle, DX_PLAYTYPE_BACK);

                    pendingBonusType = queuedBonusType;
                    queuedBonusType = 0;
                    atariLamp = false;
                    gogoLamp = (pendingBonusType != 0);
                    reel.StartSpin(pendingBonusType);
                    totalGames++;
                    resultChecked = false;
                }
                else if (selectedBet > 0 && medals >= selectedBet)
                {
                    currentBet = selectedBet;
                    medals -= currentBet;
                    PlaySoundMem(startSoundHandle, DX_PLAYTYPE_BACK);

                    pendingBonusType = queuedBonusType;
                    queuedBonusType = 0;
                    atariLamp = false;
                    gogoLamp = (pendingBonusType != 0);
                    reel.StartSpin(pendingBonusType);
                    selectedBet = 0;
                    totalGames++;
                    resultChecked = false;
                }
            }
        }

        //停止ボタン。左から順に0、1、2
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
    DeleteGraph(normalLampImage);
    DeleteGraph(hitLampImage);

    reel.ReleaseImages();

    DxLib_End();
    return 0;
}