#include "../sierrachart.h"
#include <windows.h>

SCDLLName("ActiveChartBorder")

void DrawToChart(
    HWND WindowHandle,
    HDC DeviceContext,
    SCStudyInterfaceRef sc);

inline bool IsChartWindowActive(HWND ChartWindow)
{
    const HWND focusWindow = GetFocus();
    return (focusWindow == ChartWindow);
}

constexpr n_ACSIL::s_GraphicsPen::e_PenStyle GetPenStyle(int16_t LineStyle) noexcept
{
    switch (LineStyle)
    {
    case LINESTYLE_SOLID:
        return n_ACSIL::s_GraphicsPen::e_PenStyle::PEN_STYLE_SOLID;

    case LINESTYLE_DASH:
        return n_ACSIL::s_GraphicsPen::e_PenStyle::PEN_STYLE_DASH;

    case LINESTYLE_DOT:
        return n_ACSIL::s_GraphicsPen::e_PenStyle::PEN_STYLE_DOT;

    case LINESTYLE_DASHDOT:
        return n_ACSIL::s_GraphicsPen::e_PenStyle::PEN_STYLE_DASHDOT;

    case LINESTYLE_DASHDOTDOT:
        return n_ACSIL::s_GraphicsPen::e_PenStyle::PEN_STYLE_DASHDOTDOT;

    case LINESTYLE_ALTERNATE:
    case LINESTYLE_UNSET:
    default:
        return n_ACSIL::s_GraphicsPen::e_PenStyle::PEN_STYLE_SOLID;
    }
}

SCSFExport scsf_ActiveChartBorder(SCStudyInterfaceRef sc)
{
    if (sc.SetDefaults)
    {
        sc.GraphName = "Active Chart Border";

        sc.GraphRegion = 0;
        sc.AutoLoop = 0;
        sc.UpdateAlways = 1;

        sc.DisplayStudyInputValues = 0;
        sc.DisplayStudyName = 0;
        sc.GlobalDisplayStudySubgraphsNameAndValue = 0;

        sc.Input[0].Name = "Enable active chart detection";
        sc.Input[0].SetYesNo(false);

        // Pre-configure all 4 border subgraphs
        constexpr const char *const SubgraphNames[4] = {
            "Top Border",
            "Bottom Border",
            "Left Border",
            "Right Border"};

        for (int i = 0; i < 4; ++i)
        {
            sc.Subgraph[i].Name = SubgraphNames[i];
            sc.Subgraph[i].DrawStyle = DRAWSTYLE_LINE;
            sc.Subgraph[i].LineStyle = LINESTYLE_SOLID;
            sc.Subgraph[i].PrimaryColor = RGB(255, 174, 66);
            sc.Subgraph[i].LineWidth = 10;
            sc.Subgraph[i].GraphicalDisplacement = 0;
        }

        return;
    }

    sc.p_GDIFunction = DrawToChart;
}

void DrawToChart(
    HWND WindowHandle,
    HDC DeviceContext,
    SCStudyInterfaceRef sc)
{
    if (WindowHandle == nullptr)
        return;

    // Fast check: only draw when active if enabled
    if (sc.Input[0].GetYesNo() && !IsChartWindowActive(WindowHandle))
        return;

    RECT ClientRect;
    if (!GetClientRect(WindowHandle, &ClientRect))
        return;

    const int windowLeft = ClientRect.left;
    const int windowTop = ClientRect.top;
    const int windowRight = ClientRect.right - 1;
    const int windowBottom = ClientRect.bottom - 1;

    // Validate client area bounds
    if (windowRight <= windowLeft || windowBottom <= windowTop)
        return;

    const int top = windowTop + sc.Subgraph[0].GraphicalDisplacement;
    const int bottom = windowBottom - sc.Subgraph[1].GraphicalDisplacement;
    const int left = windowLeft + sc.Subgraph[2].GraphicalDisplacement;
    const int right = windowRight - sc.Subgraph[3].GraphicalDisplacement;

    if (left > right || top > bottom)
        return;

    // Pen state caching to eliminate redundant GDI pen creations and state switches
    COLORREF lastColor = CLR_INVALID;
    int16_t lastLineStyle = -1;
    int lastLineWidth = -1;

    auto DrawLine = [&](int x1, int y1, int x2, int y2, const SCSubgraphRef &Subgraph)
    {
        if (Subgraph.DrawStyle != DRAWSTYLE_LINE)
            return;

        const COLORREF color = Subgraph.PrimaryColor;
        const int16_t lineStyle = Subgraph.LineStyle;
        const int lineWidth = Subgraph.LineWidth;

        // Only update GDI pen when pen attributes actually differ
        if (color != lastColor || lineStyle != lastLineStyle || lineWidth != lastLineWidth)
        {
            n_ACSIL::s_GraphicsPen Pen;
            Pen.m_PenColor.SetRGB(
                GetRValue(color),
                GetGValue(color),
                GetBValue(color));
            Pen.m_PenStyle = GetPenStyle(lineStyle);
            Pen.m_Width = lineWidth;

            sc.Graphics.SetPen(Pen);

            lastColor = color;
            lastLineStyle = lineStyle;
            lastLineWidth = lineWidth;
        }

        sc.Graphics.MoveTo(x1, y1);
        sc.Graphics.LineTo(x2, y2);
    };

    // Draw borders
    DrawLine(left, top, right, top, sc.Subgraph[0]);       // Top
    DrawLine(left, bottom, right, bottom, sc.Subgraph[1]); // Bottom
    DrawLine(left, top, left, bottom, sc.Subgraph[2]);     // Left
    DrawLine(right, top, right, bottom, sc.Subgraph[3]);   // Right
}
