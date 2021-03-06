#include "stdafx.h"


HRESULT STDMETHODCALLTYPE toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::QueryInterface(REFIID riid, LPVOID FAR *ppvObject)
{
	if (ppvObject == NULL) return E_INVALIDARG;
	*ppvObject = NULL;
	if (riid == IID_IUnknown) { AddRef(); *ppvObject = (IUnknown*)this; return S_OK; }
	else if (riid == IID_IDropTarget) { AddRef(); *ppvObject = (IDropTarget*)this; return S_OK; }
	else return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE   toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::AddRef()
{
	return InterlockedIncrement(&drop_ref_count);
}
ULONG STDMETHODCALLTYPE   toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::Release()
{
	LONG rv = InterlockedDecrement(&drop_ref_count);
	if (!rv)
		delete this;
	return rv;
}
bool toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::check_do(IDataObject *pDO)
{
	bool rv = false;
	FORMATETC fe;
	memset(&fe, 0, sizeof(fe));
	fe.cfFormat = g_clipformat();
	fe.dwAspect = DVASPECT_CONTENT;
	fe.lindex = -1;
	fe.tymed = TYMED_HGLOBAL;
	//console::formatter() << "check_do: " << (int)fe.cfFormat;
	STGMEDIUM sm;
	memset(&sm, 0, sizeof(sm));
	if (SUCCEEDED(pDO->GetData(&fe, &sm)))
	{
		DDData * pDDD = (DDData*)GlobalLock(sm.hGlobal);
		if (pDDD && pDDD->version == 0 && pDDD->wnd == m_button_list_view->get_wnd()) rv = true;
		ReleaseStgMedium(&sm);
	}
	return rv;
}
HRESULT STDMETHODCALLTYPE toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	//console::formatter() << "DragEnter";
	m_DataObject = pDataObj;
	last_rmb = ((grfKeyState & MK_RBUTTON) != 0);
	POINT pt = { ptl.x, ptl.y };
	if (m_DropTargetHelper.is_valid()) m_DropTargetHelper->DragEnter(m_button_list_view->get_wnd(), pDataObj, &pt, *pdwEffect);
	*pdwEffect = DROPEFFECT_NONE;
	if (check_do(pDataObj/*, pdwEffect*/))
	{
		mmh::ole::SetDropDescription(m_DataObject.get_ptr(), DROPIMAGE_MOVE, "Move", "");
		*pdwEffect = DROPEFFECT_MOVE;
	}
	return S_OK;
}
HRESULT STDMETHODCALLTYPE toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	//console::formatter() << "DragOver";
	POINT pt = { ptl.x, ptl.y };
	if (m_DropTargetHelper.is_valid()) m_DropTargetHelper->DragOver(&pt, *pdwEffect);

	last_rmb = ((grfKeyState & MK_RBUTTON) != 0);
	POINT pti;
	pti.y = ptl.y;
	pti.x = ptl.x;

	*pdwEffect = DROPEFFECT_NONE;
	if (check_do(m_DataObject/*, pdwEffect*/))
	{
		*pdwEffect = DROPEFFECT_MOVE;
		HRESULT hr = mmh::ole::SetDropDescription(m_DataObject.get_ptr(), DROPIMAGE_MOVE, "Move", "");
		//console::formatter() << pfc::format_hex(hr);

	}
	if (m_button_list_view->get_wnd())
	{
		t_list_view::t_hit_test_result hi;

		{
			POINT ptt = pti;
			ScreenToClient(m_button_list_view->get_wnd(), &ptt);

			RECT rc_items;
			m_button_list_view->get_items_rect(&rc_items);

			rc_items.top += m_button_list_view->get_item_height();
			rc_items.bottom -= m_button_list_view->get_item_height();

			if (ptt.y < rc_items.top && ptt.y < rc_items.bottom)
			{
				m_button_list_view->create_timer_scroll_up();
			}
			else m_button_list_view->destroy_timer_scroll_up();

			if (ptt.y >= rc_items.top && ptt.y >= rc_items.bottom)
			{
				m_button_list_view->create_timer_scroll_down();
			}
			else m_button_list_view->destroy_timer_scroll_down();
			{
				m_button_list_view->hit_test_ex(ptt, hi, true);
				if (hi.result == t_list_view::hit_test_on || hi.result == t_list_view::hit_test_on_group
					|| hi.result == t_list_view::hit_test_obscured_below)
					m_button_list_view->set_insert_mark(hi.index);
				else if (hi.result == t_list_view::hit_test_below_items)
					m_button_list_view->set_insert_mark(hi.index + 1);
				else
					m_button_list_view->remove_insert_mark();
			}
		}
	}

	return S_OK;
}
HRESULT STDMETHODCALLTYPE toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::DragLeave()
{
	//console::formatter() << "DragLeave";
	if (m_DropTargetHelper.is_valid()) m_DropTargetHelper->DragLeave();
	mmh::ole::SetDropDescription(m_DataObject.get_ptr(), DROPIMAGE_INVALID, "", "");
	m_button_list_view->remove_insert_mark();
	m_button_list_view->destroy_timer_scroll_up();
	m_button_list_view->destroy_timer_scroll_down();
	m_DataObject.release();
	return S_OK;
}
HRESULT STDMETHODCALLTYPE toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
	POINT pt = { ptl.x, ptl.y };
	if (m_DropTargetHelper.is_valid()) m_DropTargetHelper->Drop(pDataObj, &pt, *pdwEffect);

	*pdwEffect = DROPEFFECT_NONE;
	if (check_do(pDataObj/*, pdwEffect*/))
	{
		*pdwEffect = DROPEFFECT_MOVE;
		t_size index = m_button_list_view->get_selected_item_single(); //meh

		t_list_view::t_hit_test_result hi;

		POINT ptt = pt;
		ScreenToClient(m_button_list_view->get_wnd(), &ptt);

		m_button_list_view->hit_test_ex(ptt, hi, true);

		t_size new_index = pfc_infinite;

		if (hi.result == t_list_view::hit_test_on || hi.result == t_list_view::hit_test_on_group
			|| hi.result == t_list_view::hit_test_obscured_below)
			new_index = hi.index;
		else if (hi.result == t_list_view::hit_test_below_items)
			new_index = hi.index + 1;

		if (new_index != pfc_infinite && new_index > index) --new_index;

		if (new_index != pfc_infinite && index != pfc_infinite && new_index != index && index < m_button_list_view->m_param.m_buttons.get_count())
		{
			t_size button_count = m_button_list_view->m_param.m_buttons.get_count(), abs_delta = abs(int(index) - int(new_index));
			int direction = new_index>index ? -1 : 1;
			mmh::permutation_t perm(button_count);
			for (t_size i = 0; i < abs_delta; i++)
				perm.swap_items(new_index + direction*(i), new_index + direction*(i + 1));

			m_button_list_view->m_param.m_buttons.reorder(perm.get_ptr());

			//blaarrgg, designed in the dark ages
			m_button_list_view->m_param.m_selection = &m_button_list_view->m_param.m_buttons[new_index];

			m_button_list_view->m_param.refresh_buttons_list_items(min(index, new_index), abs_delta + 1);
			m_button_list_view->set_item_selected_single(new_index);

		}

	}
	m_DataObject.release();
	m_button_list_view->remove_insert_mark();
	m_button_list_view->destroy_timer_scroll_up();
	m_button_list_view->destroy_timer_scroll_down();
	return S_OK;
}
toolbar_extension::config_param::t_button_list_view::IDropTarget_buttons_list::IDropTarget_buttons_list(toolbar_extension::config_param::t_button_list_view * p_blv) : drop_ref_count(0), last_rmb(false), m_button_list_view(p_blv)
{
	m_DropTargetHelper.instantiate(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER);
}


