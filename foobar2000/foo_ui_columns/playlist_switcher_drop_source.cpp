#include "stdafx.h"


bool playlist_switcher_t::do_drag_drop(WPARAM wp) 
{
	try 
	{
		bit_array_bittable mask(get_item_count());
		get_selection_state(mask);

		playlist_dataobject_desc_impl data;
		data.set_from_playlist_manager(mask);

		pfc::com_ptr_t<IDataObject> pDataObject = static_api_ptr_t<ole_interaction_v2>()->create_dataobject(data);

		if (pDataObject.is_valid())
		{
			DWORD blah = DROPEFFECT_NONE;
			pfc::com_ptr_t<mmh::ole::IDropSource_Generic> p_IDropSource = new mmh::ole::IDropSource_Generic(get_wnd(), pDataObject.get_ptr(), wp);
			m_dragging = true;
			m_DataObject = pDataObject;
			HRESULT hr = DoDragDrop(pDataObject.get_ptr(),p_IDropSource.get_ptr(),DROPEFFECT_COPY|DROPEFFECT_MOVE,&blah);
				//SHDoDragDrop(get_wnd(), pDataObject.get_ptr(),NULL,DROPEFFECT_COPY|DROPEFFECT_MOVE,&blah);
			m_DataObject.release();
			m_dragging = false;
		}
	} 
	catch (exception_service_extension_not_found const &) {;}
	catch (exception_service_not_found const &) {;}

	return true;
}


HRESULT STDMETHODCALLTYPE playlist_switcher_t::IDropSource_t::QueryInterface(REFIID iid,void ** ppvObject)
{
	if (ppvObject == NULL) return E_INVALIDARG;
	*ppvObject = NULL;
	if (iid == IID_IUnknown) {AddRef();*ppvObject = (IUnknown*)this;return S_OK;}
	else if (iid == IID_IDropSource) {AddRef();*ppvObject = (IDropSource*)this;return S_OK;}
	else return E_NOINTERFACE;
}
ULONG STDMETHODCALLTYPE playlist_switcher_t::IDropSource_t::AddRef() {return InterlockedIncrement(&refcount);}
ULONG STDMETHODCALLTYPE playlist_switcher_t::IDropSource_t::Release()
{
	LONG rv = InterlockedDecrement(&refcount);
	if (!rv)
	{
		delete this;
	}
	return rv;
}

HRESULT STDMETHODCALLTYPE playlist_switcher_t::IDropSource_t::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
	if (fEscapePressed || ((m_initial_key_state & MK_LBUTTON) && (grfKeyState&MK_RBUTTON)) ) {return DRAGDROP_S_CANCEL;}
	else if ( ((m_initial_key_state & MK_LBUTTON) && !(grfKeyState&MK_LBUTTON ))
		|| ((m_initial_key_state & MK_RBUTTON) && !(grfKeyState&MK_RBUTTON )))
	{
		return DRAGDROP_S_DROP;
	}
	else return S_OK;
}

HRESULT STDMETHODCALLTYPE playlist_switcher_t::IDropSource_t::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

playlist_switcher_t::IDropSource_t::IDropSource_t(playlist_switcher_t * p_window, DWORD initial_key_state) : refcount(0), m_window(p_window),
m_initial_key_state(initial_key_state) {};

