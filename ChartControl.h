#pragma once

// Сформированные компьютером классы-оболочки IDispatch, созданные при помощи Microsoft Visual C++

// Примечание. Не изменяйте содержимое этого файла. При повторном создании этого класса
// в Microsoft Visual C++ изменения будут перезаписаны.

/////////////////////////////////////////////////////////////////////////////

#include "afxwin.h"

class ChartControl : public CWnd
{
protected:
	DECLARE_DYNCREATE(ChartControl)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= {0xc9c6d8d6, 0x5d04, 0x4057, {0xb8, 0x51, 0x38, 0x70, 0xf0, 0x83, 0xe2, 0xb9}};
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
						const RECT& rect, CWnd* pParentWnd, UINT nID, 
						CCreateContext* pContext = nullptr)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID);
	}

	BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd,
				UINT nID, CFile* pPersist = nullptr, BOOL bStorage = FALSE,
				BSTR bstrLicKey = nullptr)
	{ 
		return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); 
	}

// Атрибуты
public:


// Операции
public:
// _DChartControl

// Функции
//

	void AboutBox()
	{
		InvokeHelper(DISPID_ABOUTBOX, DISPATCH_METHOD, VT_EMPTY, nullptr, nullptr);
	}

	void Clear()
	{
		InvokeHelper(0x1, DISPATCH_METHOD, VT_EMPTY, nullptr, nullptr);
	}

	void LoadData(VARIANT varRows)
	{
		static BYTE parms[] = VTS_VARIANT;
		InvokeHelper(0x3, DISPATCH_METHOD, VT_EMPTY, nullptr, parms, &varRows);
	}

	void ExportToExcel()
	{
		InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, nullptr, nullptr);
	}

	void ExportToWord()
	{
		InvokeHelper(0x6, DISPATCH_METHOD, VT_EMPTY, nullptr, nullptr);
	}

	void Sort(signed char asc)
	{
		static BYTE parms[] = VTS_I1;
		InvokeHelper(0x10, DISPATCH_METHOD, VT_EMPTY, nullptr, parms, asc);
	}

// Свойства
//

	CString GetChartTitle()
	{
		CString result;
		GetProperty(0x7, VT_BSTR, (void*)&result);
		return result;
	}

	void SetChartTitle(CString propVal)
	{
		SetProperty(0x7, VT_BSTR, propVal);
	}

	signed char GetShowLabels()
	{
		signed char result;
		GetProperty(0x8, VT_I1, (void*)&result);
		return result;
	}

	void SetShowLabels(signed char propVal)
	{
		SetProperty(0x8, VT_I1, propVal);
	}

	double GetCoefA()
	{
		double result;
		GetProperty(0x9, VT_R8, (void*)&result);
		return result;
	}

	void SetCoefA(double propVal)
	{
		SetProperty(0x9, VT_R8, propVal);
	}

	double GetCoefB()
	{
		double result;
		GetProperty(0xA, VT_R8, (void*)&result);
		return result;
	}

	void SetCoefB(double propVal)
	{
		SetProperty(0xA, VT_R8, propVal);
	}

	VARIANT GetColors()
	{
		VARIANT result;
		GetProperty(0x2, VT_VARIANT, (void*)&result);
		return result;
	}

	void SetColors(VARIANT propVal)
	{
		SetProperty(0x2, VT_VARIANT, (VARIANT*)&propVal);
	}

	signed char GetEnable3D()
	{
		signed char result;
		GetProperty(0xB, VT_I1, (void*)&result);
		return result;
	}

	void SetEnable3D(signed char propVal)
	{
		SetProperty(0xB, VT_I1, propVal);
	}

	signed char GetShowGrid()
	{
		signed char result;
		GetProperty(0xC, VT_I1, (void*)&result);
		return result;
	}

	void SetShowGrid(signed char propVal)
	{
		SetProperty(0xC, VT_I1, propVal);
	}

	unsigned long GetGridColor()
	{
		unsigned long result;
		GetProperty(0xD, VT_UI4, (void*)&result);
		return result;
	}

	void SetGridColor(unsigned long propVal)
	{
		SetProperty(0xD, VT_UI4, propVal);
	}

	unsigned long GetAxisColor()
	{
		unsigned long result;
		GetProperty(0xE, VT_UI4, (void*)&result);
		return result;
	}

	void SetAxisColor(unsigned long propVal)
	{
		SetProperty(0xE, VT_UI4, propVal);
	}

	unsigned long GetBackColor()
	{
		unsigned long result;
		GetProperty(0xF, VT_UI4, (void*)&result);
		return result;
	}

	void SetBackColor(unsigned long propVal)
	{
		SetProperty(0xF, VT_UI4, propVal);
	}



};
