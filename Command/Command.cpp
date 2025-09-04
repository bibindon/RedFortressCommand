#include "Command.h"

#include <sstream>
#include <algorithm>

#include "HeaderOnlyCsv.hpp"

using namespace NSCommand;

static std::vector<std::wstring> split(const std::wstring& s, wchar_t delim)
{
    std::vector<std::wstring> result;
    std::wstringstream ss(s);
    std::wstring item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

void Command::Init(IFont* font, ISoundEffect* pSE, ISprite* sprCursor, const bool bEnglish,
                      const std::wstring csvfile)
{
    m_font = font;
    m_SE = pSE;
    m_sprCursor = sprCursor;

    m_font->Init(bEnglish);
    m_SE->Init();

    auto vvws = csv::Read(csvfile);
    vvws.erase(vvws.begin());

    for (auto& ws : vvws)
    {
        m_nameMap[ws.at(0)] = ws.at(1);
    }
}

void NSCommand::Command::Finalize()
{
    delete m_font;
    delete m_SE;
    delete m_sprCursor;
}

void NSCommand::Command::UpsertCommand(const std::wstring& id,
                                          const bool enable)
{
    auto result = std::find_if(m_commandList.begin(), m_commandList.end(),
                               [&](const CommandItem& x)
                               {
                                   return x.GetId() == id;
                               });

    if (result != m_commandList.end())
    {
        result->SetEnable(enable);
    }
    else
    {
        CommandItem CommandItem;
        CommandItem.SetId(id);
        CommandItem.SetName(m_nameMap.at(id));
        CommandItem.SetEnable(enable);

        m_commandList.push_back(CommandItem);
    }

    ResetRect();
}

void NSCommand::Command::RemoveCommand(const std::wstring& id)
{
    auto result = std::remove_if(m_commandList.begin(), m_commandList.end(),
                                 [&](const CommandItem& x)
                                 {
                                     return x.GetId() == id;
                                 });

    m_commandList.erase(result, m_commandList.end());

    ResetRect();
}

void NSCommand::Command::RemoveAll()
{
    m_commandList.clear();
}

void Command::Draw()
{
    // もし、選択不可能なコマンド上にカーソルがあったら、
    // 選択可能なコマンド上にカーソルを移動させる。

    if ((int)m_commandList.size() <= m_cursorIndex ||
        !m_commandList.at(m_cursorIndex).GetEnable())
    {
        m_cursorIndex = 0;

        while (true)
        {
            if (m_commandList.at(m_cursorIndex).GetEnable() == false)
            {
                m_cursorIndex++;
                if (m_cursorIndex >= (int)m_commandList.size())
                {
                    m_cursorIndex =  -1;
                    break;
                }
                continue;
            }
            else
            {
                break;
            }
        }
    }

    // コマンドを中央揃えで表示する
    for (int i = 0; i < (int)m_commandList.size(); ++i)
    {
        bool enable = m_commandList.at(i).GetEnable();
        // 選択可能コマンド
        if (enable)
        {
            // 選択中コマンド
            if (m_cursorIndex == i)
            {
                m_font->DrawText_(m_commandList.at(i).GetName(),
                                  m_commandList.at(i).GetLeftPos(),
                                  STARTY,
                                  255);
            }
            // 未選択コマンド
            else
            {
                m_font->DrawText_(m_commandList.at(i).GetName(),
                                  m_commandList.at(i).GetLeftPos(),
                                  STARTY,
                                  128);
            }
        }
        // 使用不可コマンド
        else
        {
            m_font->DrawText_(m_commandList.at(i).GetName(),
                              m_commandList.at(i).GetLeftPos(),
                              STARTY,
                              64);
        }

        // カーソルの表示
        if (m_cursorIndex == i)
        {
            int x = 0;
            x = m_commandList.at(i).GetLeftPos();
            // 100/2ピクセル右にずらす。丸の半径が10ピクセルくらいなので少し左に戻す。
            x += (COMMAND_WIDTH / 2) - 5;
            m_sprCursor->DrawImage(x, STARTY + CURSOR_PADDING_Y);
        }
    }
}

void NSCommand::Command::Previous()
{
    auto it = std::find_if(m_commandList.begin(), m_commandList.end(),
                           [](const CommandItem& x)
                           {
                               return x.GetEnable();
                           });
    if (it == m_commandList.end())
    {
        return;
    }

    while (true)
    {
        --m_cursorIndex;
        if (m_cursorIndex <= -1)
        {
            m_cursorIndex = (int)m_commandList.size() - 1;
        }

        if (m_commandList.at(m_cursorIndex).GetEnable() == false)
        {
            continue;
        }
        else
        {
            break;
        }
    }
    m_SE->PlayMove();
}

void NSCommand::Command::Next()
{
    auto it = std::find_if(m_commandList.begin(), m_commandList.end(),
                           [](const CommandItem& x)
                           {
                               return x.GetEnable();
                           });
    if (it == m_commandList.end())
    {
        return;
    }

    while (true)
    {
        ++m_cursorIndex;
        if (m_cursorIndex >= (int)m_commandList.size())
        {
            m_cursorIndex = 0;
        }

        if (m_commandList.at(m_cursorIndex).GetEnable() == false)
        {
            continue;
        }
        else
        {
            break;
        }
    }
    m_SE->PlayMove();
}

std::wstring NSCommand::Command::Into()
{
    m_SE->PlayClick();
    return m_commandList.at(m_cursorIndex).GetId();
}

void NSCommand::Command::MouseMove(const int x, const int y)
{
    int index = -1;
    for (int i = 0; i < (int)m_commandList.size(); ++i)
    {
        int top = 0;
        int left = 0;
        int bottom = 0;
        int right = 0;
        m_commandList.at(i).GetRect(&top, &left, &bottom, &right);

        if (left <= x && x <= right)
        {
            if (top <= y && y <= bottom)
            {
                index = i;
                break;
            }
        }
    }
    if (index != -1 && m_cursorIndex != index)
    {
        if (m_commandList.at(index).GetEnable())
        {
            m_cursorIndex = index;
            m_SE->PlayMove();
        }
    }
}

std::wstring NSCommand::Command::Click(const int x, const int y)
{
    int index = -1;
    for (int i = 0; i < (int)m_commandList.size(); ++i)
    {
        int top = 0;
        int left = 0;
        int bottom = 0;
        int right = 0;
        m_commandList.at(i).GetRect(&top, &left, &bottom, &right);

        if (left <= x && x <= right)
        {
            if (top <= y && y <= bottom)
            {
                index = i;
                break;
            }
        }
    }

    if (index != -1)
    {
        if (m_commandList.at(index).GetEnable())
        {
            m_cursorIndex = index;
            m_SE->PlayClick();
        }
    }

    if (index == -1)
    {
        return std::wstring();
    }
    else
    {
        return m_commandList.at(m_cursorIndex).GetId();
    }
}

void NSCommand::Command::OnDeviceLost()
{
    m_font->OnDeviceLost();
    m_sprCursor->OnDeviceLost();
}

void NSCommand::Command::OnDeviceReset()
{
    m_font->OnDeviceReset();
    m_sprCursor->OnDeviceReset();
}

void NSCommand::Command::ResetRect()
{
    // 奇数の場合
    if (m_commandList.size() % 2 == 1)
    {
        for (int i = 0; i < (int)m_commandList.size(); ++i)
        {
            int top = 0;
            int left = 0;
            int bottom = 0;
            int right = 0;

            top = STARTY;
            bottom = STARTY + COMMAND_HEIGHT;

            left = CENTERX;
            left += INTERVAL * (i - ((int)m_commandList.size() / 2));
            right = left + COMMAND_WIDTH;

            m_commandList.at(i).SetRect(top, left, bottom, right);
        }
    }
    // 偶数の場合
    else
    {
        for (int i = 0; i < (int)m_commandList.size(); ++i)
        {
            int top = 0;
            int left = 0;
            int bottom = 0;
            int right = 0;

            top = STARTY;
            bottom = STARTY + COMMAND_HEIGHT;

            left = CENTERX;
            left += INTERVAL * (i - ((int)m_commandList.size() / 2));
            left += INTERVAL / 2;
            right = left + COMMAND_WIDTH;

            m_commandList.at(i).SetRect(top, left, bottom, right);
        }
    }
}

void NSCommand::CommandItem::SetName(const std::wstring& arg)
{
    m_name = arg;
}

std::wstring NSCommand::CommandItem::GetName() const
{
    return m_name;
}

void NSCommand::CommandItem::SetEnable(const bool arg)
{
    m_bEnable = arg;
}

bool NSCommand::CommandItem::GetEnable() const
{
    return m_bEnable;
}

void NSCommand::CommandItem::SetRect(const int top, const int left, const int bottom, const int right)
{
    m_top = top;
    m_left = left;
    m_bottom = bottom;
    m_right = right;
}

void NSCommand::CommandItem::GetRect(int* top, int* left, int* bottom, int* right)
{
    *top = m_top;
    *left = m_left;
    *bottom = m_bottom;
    *right = m_right;
}

int NSCommand::CommandItem::GetLeftPos()
{
    return m_left;
}
