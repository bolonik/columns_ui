#include "stdafx.h"

// {6F000FC4-3F86-4fc5-80EA-F7AA4D9551E6}
const GUID g_guid_splitter_tabs = 
{ 0x6f000fc4, 0x3f86, 0x4fc5, { 0x80, 0xea, 0xf7, 0xaa, 0x4d, 0x95, 0x51, 0xe6 } };

class splitter_window_tabs_impl::splitter_host_impl : public ui_extension::window_host
{
	service_ptr_t<splitter_window_tabs_impl > m_this;
public:
	virtual const GUID & get_host_guid()const
	{
		// {B5C88724-EDCD-46a1-90B9-C298309FDFB7}
		static const GUID rv = 
		{ 0xb5c88724, 0xedcd, 0x46a1, { 0x90, 0xb9, 0xc2, 0x98, 0x30, 0x9f, 0xdf, 0xb7 } };
		return rv;
	}

	virtual bool get_keyboard_shortcuts_enabled()const
	{
		return m_this->get_host()->get_keyboard_shortcuts_enabled();
	}

	virtual void on_size_limit_change(HWND wnd, unsigned flags)
	{
		unsigned index;
		if (m_this->m_panels.find_by_wnd(wnd, index))
		{
			pfc::refcounted_object_ptr_t<splitter_window_tabs_impl::panel> p_ext = m_this->m_panels[index];
			MINMAXINFO mmi;
			memset(&mmi, 0, sizeof(MINMAXINFO));
			mmi.ptMaxTrackSize.x = MAXLONG;
			mmi.ptMaxTrackSize.y = MAXLONG;
			SendMessage(wnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
			p_ext->m_size_limits.min_width = min (mmi.ptMinTrackSize.x, MAXSHORT);
			p_ext->m_size_limits.min_height = min (mmi.ptMinTrackSize.y, MAXSHORT);
			p_ext->m_size_limits.max_height = min (mmi.ptMaxTrackSize.y, MAXSHORT);
			p_ext->m_size_limits.max_width = min (mmi.ptMaxTrackSize.x, MAXSHORT);

			m_this->update_size_limits();

			m_this->get_host()->on_size_limit_change(m_this->get_wnd(), flags);

			m_this->on_size_changed();
		}
	};

	virtual unsigned is_resize_supported(HWND wnd)const
	{
		return false;
	}

	virtual bool request_resize(HWND wnd, unsigned flags, unsigned width, unsigned height)
	{
		return false;
	}

	virtual bool override_status_text_create(service_ptr_t<ui_status_text_override> & p_out)
	{
		return m_this->get_host()->override_status_text_create(p_out);
	}

	virtual bool is_visible(HWND wnd) const
	{
		bool rv = false;

		if (!m_this->get_host()->is_visible(m_this->get_wnd()))
		{
			rv = false;
		}
		else 
		{
			rv = IsWindowVisible(wnd) != 0;
		}
		return  rv;
	}

	virtual bool is_visibility_modifiable(HWND wnd, bool desired_visibility) const
	{
		bool rv = false;

		if (desired_visibility)
		{
			bool b_found = false;
			unsigned idx = 0;
			b_found = (m_this->m_active_panels.find_by_wnd(wnd, idx));

			if (!m_this->get_host()->is_visible(m_this->get_wnd()))
				rv = m_this->get_host()->is_visibility_modifiable(m_this->get_wnd(), desired_visibility) && b_found;
			else 
				rv = b_found;
		}
		return  rv;
	}
	virtual bool set_window_visibility(HWND wnd, bool visibility)
	{
		bool rv = false, b_usvisible=true;
		if (!m_this->get_host()->is_visible(m_this->get_wnd()))
			b_usvisible = m_this->get_host()->set_window_visibility(m_this->get_wnd(), visibility);
		if (b_usvisible && visibility)
		{
			unsigned idx = 0;
			if (m_this->m_active_panels.find_by_wnd(wnd, idx))
			{
				{
					m_this->on_active_tab_changing(TabCtrl_GetCurSel(m_this->m_wnd_tabs));
					TabCtrl_SetCurSel(m_this->m_wnd_tabs, idx);
					m_this->on_active_tab_changed(TabCtrl_GetCurSel(m_this->m_wnd_tabs));
					//m_this->m_panels[idx]->m_hidden = !visibility;
					//m_this->get_host()->on_size_limit_change(m_this->get_wnd(), uie::size_limit_all);
					//m_this->on_size_changed();
				}
				rv=true;
			}
		}

		return rv;
	}

	void set_window_ptr(splitter_window_tabs_impl * p_ptr)
	{
		m_this = p_ptr;
	}

	virtual void relinquish_ownership(HWND wnd)
	{
		unsigned index;
		if (m_this->m_active_panels.find_by_wnd(wnd, index))
		{
			pfc::refcounted_object_ptr_t<splitter_window_tabs_impl::panel> p_ext = m_this->m_active_panels[index];

			{
				/*if (GetAncestor(wnd, GA_PARENT) == m_this->get_wnd())
				{
				console::warning("window left by ui extension");
				SetParent(wnd, core_api::get_main_window());
				}*/

				p_ext->m_wnd=0;
				p_ext->m_child.release();
				m_this->m_active_panels.remove_by_idx(index);
				m_this->m_panels.remove_item(p_ext);
				TabCtrl_DeleteItem(m_this->m_wnd_tabs, index);
				m_this->update_size_limits();
				m_this->get_host()->on_size_limit_change(m_this->get_wnd(), uie::size_limit_all);

				m_this->on_size_changed();
			}
		}
	}

};

ui_extension::window_host_factory<splitter_window_tabs_impl::splitter_host_impl > g_splitter_tabs_host;

void splitter_window_tabs_impl::get_supported_panels(const pfc::list_base_const_t<uie::window::ptr> & p_windows, bit_array_var & p_mask_unsupported)
{
	service_ptr_t<service_base> temp;
	g_splitter_tabs_host.instance_create(temp);
	uie::window_host_ptr ptr;
	if (temp->service_query_t(ptr))
		(static_cast<splitter_window_tabs_impl::splitter_host_impl*>(ptr.get_ptr()))->set_window_ptr(this);
	t_size i, count = p_windows.get_count();
	for(i=0;i<count;i++)
		p_mask_unsupported.set(i, !p_windows[i]->is_available(ptr));
}


void clip_minmaxinfo(MINMAXINFO & mmi);

bool splitter_window_tabs_impl::panel_list::find_by_wnd(HWND wnd, unsigned & p_out)
{
	unsigned n, count = get_count();
	for (n=0; n<count; n++)
	{
		if (get_item(n)->m_wnd == wnd)
		{
			p_out = n;
			return true;
		}
	}
	return false;
}
splitter_window_tabs_impl::panel::panel()
: m_wnd(NULL), m_use_custom_title(false)
{};

splitter_window_tabs_impl::panel::~panel()
{
}

uie::splitter_item_full_t * splitter_window_tabs_impl::panel::create_splitter_item()
{
	uie::splitter_item_full_impl_t * ret = new uie::splitter_item_full_impl_t;
	ret->set_panel_guid(m_guid);
	ret->set_panel_config(&stream_reader_memblock_ref(m_child_data.get_ptr(), m_child_data.get_size()), m_child_data.get_size());
	ret->set_window_ptr(m_child);
	ret->m_custom_title = m_use_custom_title;
	ret->set_title(m_custom_title, m_custom_title.length());

	ret->m_autohide = false;
	ret->m_caption_orientation = 0;
	ret->m_locked = false;
	ret->m_hidden = false;
	ret->m_show_toggle_area = false;
	ret->m_size = 0;
	ret->m_show_caption = true;
	return ret;
}

void splitter_window_tabs_impl::panel::set_from_splitter_item(const uie::splitter_item_t * p_source)
{
	if (m_wnd) destroy();
	const uie::splitter_item_full_t * ptr = NULL;
	if (p_source->query(ptr))
	{
		m_use_custom_title = ptr->m_custom_title;
		ptr->get_title(m_custom_title);
	}
	m_child = p_source->get_window_ptr();
	m_guid = p_source->get_panel_guid();
	p_source->get_panel_config(&stream_writer_memblock_ref(m_child_data, true));
}

void splitter_window_tabs_impl::panel::destroy()
{
	if (m_child.is_valid())
	{
		m_child->destroy_window();
		m_wnd = NULL;
		m_child.release();
	}
	m_this.release();
	m_interface.release();
}

void splitter_window_tabs_impl::panel::read(stream_reader*t, abort_callback & p_abort)
{
	t->read_lendian_t(m_guid, p_abort);
	unsigned size;
	t->read_lendian_t(size, p_abort);
	m_child_data.set_size(size);
	t->read(m_child_data.get_ptr(), size, p_abort);
	t->read_lendian_t(m_use_custom_title, p_abort);
	t->read_string(m_custom_title, p_abort);
}

void splitter_window_tabs_impl::panel::write(stream_writer * out, abort_callback & p_abort)
{
	if (m_child.is_valid())
	{
		m_child_data.set_size(0);
		m_child->get_config(&stream_writer_memblock_ref(m_child_data), p_abort);
	}
	out->write_lendian_t(m_guid, p_abort);
	out->write_lendian_t(m_child_data.get_size(), p_abort);
	out->write(m_child_data.get_ptr(), m_child_data.get_size(), p_abort);
	out->write_lendian_t(m_use_custom_title, p_abort);
	out->write_string(m_custom_title, p_abort);
}
void splitter_window_tabs_impl::panel::export(stream_writer * out, abort_callback & p_abort)
{
	stream_writer_memblock child_exported_data;
	uie::window_ptr ptr = m_child;
	if (!ptr.is_valid())
	{
		if (!uie::window::create_by_guid(m_guid, ptr))
			throw cui::fcl::exception_missing_panel();
		try {
			ptr->set_config(&stream_reader_memblock_ref(m_child_data.get_ptr(), m_child_data.get_size()), m_child_data.get_size(), p_abort);
		} catch (const exception_io &) {};
	}
	{
		ptr->export_config(&child_exported_data, p_abort);
	}
	out->write_lendian_t(m_guid, p_abort);
	out->write_lendian_t(child_exported_data.m_data.get_size(), p_abort);
	out->write(child_exported_data.m_data.get_ptr(), child_exported_data.m_data.get_size(), p_abort);
	out->write_lendian_t(m_use_custom_title, p_abort);
	out->write_string(m_custom_title, p_abort);
}
void splitter_window_tabs_impl::panel::import(stream_reader*t, abort_callback & p_abort)
{
	t->read_lendian_t(m_guid, p_abort);
	unsigned size;
	t->read_lendian_t(size, p_abort);
	pfc::array_t<t_uint8> data;
	data.set_size(size);
	t->read(data.get_ptr(), size, p_abort);
	t->read_lendian_t(m_use_custom_title, p_abort);
	t->read_string(m_custom_title, p_abort);

	if (uie::window::create_by_guid(m_guid, m_child))
	{
		try {
			m_child->import_config(&stream_reader_memblock_ref(data.get_ptr(), data.get_size()), data.get_size(), p_abort);
		} catch (const exception_io &) {};
		m_child_data.set_size(0);
		m_child->get_config(&stream_writer_memblock_ref(m_child_data), p_abort);
	}
	//else
	//	throw pfc::exception_not_implemented();
}

splitter_window_tabs_impl::class_data & splitter_window_tabs_impl::get_class_data()const 
{
	__implement_get_class_data_ex(_T("{5CB67C98-B77F-4926-A79F-49D9B21B9705}"),_T(""), false,0,WS_CHILD|WS_CLIPCHILDREN, WS_EX_CONTROLPARENT,CS_DBLCLKS);
}
void splitter_window_tabs_impl::get_name(pfc::string_base & p_out) const
{
	p_out = "Tab stack";
}
const GUID & splitter_window_tabs_impl::get_extension_guid() const
{
	// {5CB67C98-B77F-4926-A79F-49D9B21B9705}
	static const GUID rv = 
	{ 0x5cb67c98, 0xb77f, 0x4926, { 0xa7, 0x9f, 0x49, 0xd9, 0xb2, 0x1b, 0x97, 0x5 } };
	return rv;
}
void splitter_window_tabs_impl::get_category(pfc::string_base & p_out) const
{
	p_out = "Splitters";
}
unsigned splitter_window_tabs_impl::get_type  () const{return ui_extension::type_layout|uie::type_splitter;};


unsigned splitter_window_tabs_impl::get_panel_count()const
{
	return m_panels.get_count();
};
uie::splitter_item_t * splitter_window_tabs_impl::get_panel(unsigned index)const
{
	if (index < m_panels.get_count()) 
	{
		return m_panels[index]->create_splitter_item();
	}
	return NULL;
};

bool splitter_window_tabs_impl::get_config_item_supported(unsigned index, const GUID & p_type) const
{
	if (p_type == uie::splitter_window::bool_use_custom_title
		|| p_type == uie::splitter_window::string_custom_title)
		return true;
	return false;
}

bool splitter_window_tabs_impl::get_config_item(unsigned index, const GUID & p_type, stream_writer * p_out, abort_callback & p_abort) const
{
	if (index < m_panels.get_count())
	{
		if (p_type == uie::splitter_window::bool_use_custom_title)
		{
			p_out->write_lendian_t(m_panels[index]->m_use_custom_title, p_abort);
			return true;
		}
		else if (p_type == uie::splitter_window::string_custom_title)
		{
			p_out->write_string(m_panels[index]->m_custom_title, p_abort);
			return true;
		}
	}
	return false;
}

bool splitter_window_tabs_impl::set_config_item(unsigned index, const GUID & p_type, stream_reader * p_source, abort_callback & p_abort)
{
	if (index < m_panels.get_count())
	{
		if (p_type == uie::splitter_window::bool_use_custom_title)
		{
			p_source->read_lendian_t(m_panels[index]->m_use_custom_title, p_abort);
			return true;
		}
		else if (p_type == uie::splitter_window::string_custom_title)
		{
			p_source->read_string(m_panels[index]->m_custom_title, p_abort);
			return true;
		}
	}
	return false;
};

void splitter_window_tabs_impl::set_config(stream_reader * config, t_size p_size, abort_callback & p_abort)
{
	if (p_size)
	{
		t_uint32 version;
		config->read_lendian_t(version, p_abort);
		if (version <= stream_version_current)
		{
			m_panels.remove_all();

			config->read_lendian_t(m_active_tab, p_abort);
			unsigned count;
			config->read_lendian_t(count, p_abort);

			unsigned n;
			for (n=0; n<count; n++)
			{
				pfc::refcounted_object_ptr_t<panel> temp = new panel;
				temp->read(config, p_abort);
				m_panels.add_item(temp);
			}
		}
	}
};
void splitter_window_tabs_impl::get_config(stream_writer * out, abort_callback & p_abort) const
{
	out->write_lendian_t((t_uint32)stream_version_current, p_abort);
	unsigned n, count = m_panels.get_count();
	out->write_lendian_t(m_active_tab, p_abort);
	out->write_lendian_t(count, p_abort);
	for (n=0; n<count; n++)
	{
		m_panels[n]->write(out, p_abort);
	}
};

void splitter_window_tabs_impl::export_config(stream_writer * p_writer, abort_callback & p_abort) const
{
	p_writer->write_lendian_t((t_uint32)stream_version_current, p_abort);
	unsigned n, count = m_panels.get_count();
	p_writer->write_lendian_t(m_active_tab, p_abort);
	p_writer->write_lendian_t(count, p_abort);
	for (n=0; n<count; n++)
	{
		m_panels[n]->export(p_writer, p_abort);
	}
};

void splitter_window_tabs_impl::import_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort) 
{
	t_uint32 version;
	p_reader->read_lendian_t(version, p_abort);
	if (version <= stream_version_current)
	{
		m_panels.remove_all();

		unsigned count;
		p_reader->read_lendian_t(m_active_tab, p_abort);
		p_reader->read_lendian_t(count, p_abort);

		unsigned n;
		for (n=0; n<count; n++)
		{
			pfc::refcounted_object_ptr_t<panel> temp = new panel;
			temp->import(p_reader, p_abort);
			m_panels.add_item(temp);
		}
	}
};


void clip_sizelimit(uie::size_limit_t & mmi)
{
	mmi.max_height = min (mmi.max_height, MAXSHORT);
	mmi.min_height = min (mmi.min_height, MAXSHORT);
	mmi.max_width = min (mmi.max_width, MAXSHORT);
	mmi.min_width = min (mmi.min_width, MAXSHORT);

}

void splitter_window_tabs_impl::update_size_limits()
{
	m_size_limits = uie::size_limit_t();

	unsigned n, count = m_active_panels.get_count();

	for (n=0; n<count; n++)
	{
		MINMAXINFO mmi;
		memset(&mmi, 0, sizeof(MINMAXINFO));
		mmi.ptMaxTrackSize.x = MAXLONG;
		mmi.ptMaxTrackSize.y = MAXLONG;

		if (m_active_panels[n]->m_wnd)
		{
			SendMessage(m_active_panels[n]->m_wnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);

			m_size_limits.min_height = max((unsigned)mmi.ptMinTrackSize.y,m_size_limits.min_height);
			m_size_limits.min_width = max((unsigned)mmi.ptMinTrackSize.x,m_size_limits.min_width);
			m_size_limits.max_height = min((unsigned)mmi.ptMaxTrackSize.y,m_size_limits.max_height);
			m_size_limits.max_width = min((unsigned)mmi.ptMaxTrackSize.x,m_size_limits.max_width);
		}
		if (m_size_limits.max_width < m_size_limits.min_width)
			m_size_limits.max_width = m_size_limits.min_width;
		if (m_size_limits.max_height < m_size_limits.min_height)
			m_size_limits.max_height = m_size_limits.min_height;
	}
	clip_sizelimit(m_size_limits);
	RECT rcmin = {0, 0, m_size_limits.min_width, m_size_limits.min_height};
	RECT rcmax = {0, 0, m_size_limits.max_width, m_size_limits.max_height};
	adjust_rect(TRUE, &rcmin);
	adjust_rect(TRUE, &rcmax);
	m_size_limits.min_width = RECT_CX(rcmin);
	m_size_limits.max_width = RECT_CX(rcmax);
	m_size_limits.min_height = RECT_CY(rcmin);
	m_size_limits.max_height = RECT_CY(rcmax);
}

void splitter_window_tabs_impl::adjust_rect(bool b_larger, RECT * rc)
{
	//TabCtrl_AdjustRect(m_wnd_tabs, b_larger, rc);
	
	if (b_larger)
	{
		RECT rc_child = *rc;
		TabCtrl_AdjustRect(m_wnd_tabs, FALSE, &rc_child);
		rc_child.top = rc->top + 2;
		TabCtrl_AdjustRect(m_wnd_tabs, TRUE, &rc_child);
		*rc = rc_child;
	}
	else
	{
		RECT rc_tabs;
		rc_tabs = *rc;
		TabCtrl_AdjustRect(m_wnd_tabs, FALSE, &rc_tabs);
		rc->top = rc_tabs.top - 2;
	}
	
}

void splitter_window_tabs_impl::set_styles(bool visible)
{
	if (m_wnd_tabs)
	{
		long flags = WS_CHILD |  TCS_HOTTRACK | TCS_TABS | (0 ? TCS_MULTILINE|TCS_RIGHTJUSTIFY  : TCS_SINGLELINE) |(visible ? WS_VISIBLE : 0)|WS_CLIPSIBLINGS |WS_TABSTOP |0;

		if (uGetWindowLong(m_wnd_tabs, GWL_STYLE) != flags)
			uSetWindowLong(m_wnd_tabs, GWL_STYLE, flags);
	}
}


LRESULT splitter_window_tabs_impl::on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg)
	{
	case WM_CREATE:
		{
			create_tabs();
			SetWindowPos(m_wnd_tabs, HWND_BOTTOM, 0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			refresh_children();

			t_size activetab = m_active_tab == pfc_infinite ? 0 : m_active_tab;
			t_size activeindex = 0;
			if (activetab < m_panels.get_count())
				activeindex = m_active_panels.find_item(m_panels[activetab]);
			if (activeindex == pfc_infinite) activeindex = 0;
			TabCtrl_SetCurSel(m_wnd_tabs, activeindex);
			set_styles();

			//on_active_tab_changed(activeindex);
			m_active_tab = activeindex < m_active_panels.get_count() ? m_panels.find_item(m_active_panels[activeindex]) : pfc_infinite; 
			update_size_limits();
			on_size_changed();
			//ShowWindow(m_wnd_tabs, SW_SHOWNORMAL);
			g_windows.add_item(this);
		}
		break;
	case WM_KEYDOWN:
		{
			if (wp != VK_LEFT && wp != VK_RIGHT && get_host()->get_keyboard_shortcuts_enabled() && g_process_keydown_keyboard_shortcuts(wp)) return 0;
			else if (wp == VK_TAB)
			{
				ui_extension::window::g_on_tab(wnd);
				return 0;
			}
			SendMessage(wnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), NULL);
		}
		break;
	case WM_SYSKEYDOWN:
		if (get_host()->get_keyboard_shortcuts_enabled() && g_process_keydown_keyboard_shortcuts(wp)) return 0;
		break;
	case WM_DESTROY:
		g_windows.remove_item(this);
		destroy_children();
		destroy_tabs();
		break;
	case WM_WINDOWPOSCHANGED:
		{
			LPWINDOWPOS lpwp = (LPWINDOWPOS)lp;
			if (!(lpwp->flags & SWP_NOSIZE))
			{
				on_size_changed(lpwp->cx, lpwp->cy);
			}
		}
		break;
	/*case WM_SIZE:
		on_size_changed(LOWORD(lp), HIWORD(lp));
		break;*/
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lp;

			lpmmi->ptMinTrackSize.y = m_size_limits.min_height;
			lpmmi->ptMinTrackSize.x = m_size_limits.min_width;
			lpmmi->ptMaxTrackSize.y = m_size_limits.max_height;
			lpmmi->ptMaxTrackSize.x = m_size_limits.max_width;

			/*console::formatter() << "min height: " << m_size_limits.min_height
				<< " max height: " << m_size_limits.max_height
				<< " min width: " << m_size_limits.min_width
				<< " max width: " << m_size_limits.max_width;*/
		}
		return 0;
	case WM_SHOWWINDOW:
		{
			if (wp == TRUE && lp == NULL && m_active_tab < m_panels.get_count() && m_panels[m_active_tab]->m_wnd && !IsWindowVisible(m_panels[m_active_tab]->m_wnd))
			{
				ShowWindow(m_panels[m_active_tab]->m_wnd, SW_SHOWNORMAL);
			}
		}
		break;
	case WM_CONTEXTMENU:
		{
			enum {IDM_BASE=1};

			POINT pt = {GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
			if (pt.x == -1 && pt.y == -1)
				GetMessagePos(&pt);

			POINT pt_client = pt,pt_client_tabs = pt;

			ScreenToClient(wnd, &pt_client);

			HMENU menu = CreatePopupMenu();

			unsigned IDM_EXT_BASE=IDM_BASE;

			HWND child = ChildWindowFromPointEx(wnd, pt_client, CWP_SKIPTRANSPARENT|CWP_SKIPINVISIBLE);
			t_size index = pfc_infinite;

			bool b_found = false;
			if (HWND(wp) == m_wnd_tabs)
			{
				ScreenToClient(m_wnd_tabs, &pt_client_tabs);
				TCHITTESTINFO info;
				//memset(&info, 0, sizeof(TCHITTESTINFO));
				info.pt = pt_client_tabs;
				index = TabCtrl_HitTest(m_wnd_tabs, &info);
				if (info.flags == TCHT_ONITEM || info.flags == TCHT_ONITEMLABEL || info.flags == TCHT_ONITEMICON)
					b_found = true;
			}
			else
			{
				b_found = m_active_panels.find_by_wnd(child, index);
			}

			{
				if (b_found && index < m_active_panels.get_count())
				{
					pfc::refcounted_object_ptr_t<panel> p_panel = m_active_panels[index];

					pfc::refcounted_object_ptr_t<ui_extension::menu_hook_impl> extension_menu_nodes = new ui_extension::menu_hook_impl;

					if (p_panel->m_child.is_valid()) 
					{
						p_panel->m_child->get_menu_items(*extension_menu_nodes.get_ptr()); 
						//if (extension_menu_nodes->get_children_count() > 0)
						//	AppendMenu(menu,MF_SEPARATOR,0,0);

						extension_menu_nodes->win32_build_menu(menu, IDM_EXT_BASE, pfc_infinite - IDM_EXT_BASE);
					}
					menu_helpers::win32_auto_mnemonics(menu);

					unsigned cmd = TrackPopupMenu(menu,TPM_RIGHTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD,pt.x,pt.y,0,wnd,0);

					if (cmd >= IDM_EXT_BASE)
					{
						extension_menu_nodes->execute_by_id(cmd);
					}

					DestroyMenu(menu);
				}
			}
		}
		return 0;
	case WM_NOTIFY:
		{
			switch (((LPNMHDR)lp)->idFrom)
			{
			case 2345:
				switch (((LPNMHDR)lp)->code)
				{
				case TCN_SELCHANGING:
					on_active_tab_changing(TabCtrl_GetCurSel(m_wnd_tabs));
					break;
				case TCN_SELCHANGE:
					on_active_tab_changed(TabCtrl_GetCurSel(m_wnd_tabs));
					if (m_active_tab != pfc_infinite && m_active_tab < m_active_panels.get_count() && GetFocus() != m_wnd_tabs)
					{
						HWND wnd_root = GetAncestor(m_wnd_tabs, GA_ROOT), wnd_focus = NULL;
						if (wnd_root)
						{

							wnd_focus = m_active_panels[m_active_tab]->m_wnd;
							if (!(GetWindowLongPtr(wnd_focus, GWL_STYLE) & WS_TABSTOP))
								wnd_focus = GetNextDlgTabItem(wnd_root, m_wnd_tabs, FALSE);
							if (wnd_focus && ( IsChild(m_active_panels[m_active_tab]->m_wnd, wnd_focus) || m_active_panels[m_active_tab]->m_wnd == wnd_focus))
								SetFocus(wnd_focus);
							else
								wnd_focus = NULL;
						}
						if (!wnd_focus)
							SetFocus(m_wnd_tabs);
					}
					break;
				}
				break;
			}
		}
		break;
	}
	return DefWindowProc(wnd, msg, wp, lp);
}

void splitter_window_tabs_impl::refresh_children()
{
	unsigned n, count = m_panels.get_count();
	for (n=0; n<count; n++)
	{
		if (!m_panels[n]->m_wnd)
		{
			m_panels[n]->set_splitter_window_ptr(this);
			uie::window_ptr p_ext;
			p_ext = m_panels[n]->m_child;

			bool b_new = false;
			if (!p_ext.is_valid()) 
			{
				ui_extension::window::create_by_guid(m_panels[n]->m_guid, p_ext);
				b_new = true;
			}

			if (!m_panels[n]->m_interface.is_valid())
			{
				service_ptr_t<service_base> temp;
				g_splitter_tabs_host.instance_create(temp);
				uie::window_host_ptr ptr;
				if (temp->service_query_t(ptr))
				{
					m_panels[n]->m_interface = static_cast<splitter_host_impl*>(ptr.get_ptr());
					m_panels[n]->m_interface->set_window_ptr(this);
				}
			}


			if (p_ext.is_valid() && p_ext->is_available(uie::window_host_ptr(static_cast<uie::window_host*>(m_panels[n]->m_interface.get_ptr())))) 
			{
				pfc::string8 name;
				if (m_panels[n]->m_use_custom_title)
				{
					name = m_panels[n]->m_custom_title;
				}
				else
				{
					if (!p_ext->get_short_name(name))
						p_ext->get_name(name);
				}


				{
					if (b_new)
					{
						try {
							p_ext->set_config(&stream_reader_memblock_ref(m_panels[n]->m_child_data.get_ptr(), m_panels[n]->m_child_data.get_size()),m_panels[n]->m_child_data.get_size(),abort_callback_impl());
						}
						catch (const exception_io & e)
						{
							console::formatter() << "Error setting panel config: " << e.what();
						}
					}

					t_size index = m_active_panels.find_item(m_panels[n]);
					bool b_newtab = index == pfc_infinite;
					if (b_newtab)
						index = m_active_panels.insert_item(m_panels[n], index);
					HWND wnd_panel = p_ext->create_or_transfer_window(get_wnd(), uie::window_host_ptr(m_panels[n]->m_interface.get_ptr()));
					if (wnd_panel)
					{
						if (GetWindowLongPtr(wnd_panel, GWL_STYLE) & WS_VISIBLE)
						{
							pfc::string8 name;
							p_ext->get_name(name);
							console::formatter() << "Columns UI/Tab stack: Warning: " << name << " panel was visible on creation! This usually indicates a bug in this panel."; 
							ShowWindow(wnd_panel, SW_HIDE);
						}
						SetWindowLongPtr(wnd_panel, GWL_STYLE, GetWindowLongPtr(wnd_panel, GWL_STYLE)|WS_CLIPSIBLINGS);
						SetWindowPos(wnd_panel,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
						//uxtheme_api_ptr p_uxtheme;
						//if (p_uxtheme.load())
						//	p_uxtheme->EnableThemeDialogTexture(wnd_panel, ETDT_ENABLETAB);

						uTabCtrl_InsertItemText(m_wnd_tabs, index, name, b_newtab);

						MINMAXINFO mmi;
						memset(&mmi, 0, sizeof(MINMAXINFO));
						mmi.ptMaxTrackSize.x = MAXLONG;
						mmi.ptMaxTrackSize.y = MAXLONG;
						uSendMessage(wnd_panel, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
						clip_minmaxinfo(mmi);

						m_panels[n]->m_wnd = wnd_panel;
						m_panels[n]->m_child = p_ext;
						m_panels[n]->m_size_limits.min_height = mmi.ptMinTrackSize.y;
						m_panels[n]->m_size_limits.min_width = mmi.ptMinTrackSize.x;
						m_panels[n]->m_size_limits.max_width = mmi.ptMaxTrackSize.x;
						m_panels[n]->m_size_limits.max_height = mmi.ptMaxTrackSize.y;
					}
					else
						m_active_panels.remove_by_idx(index);
				}
			}
		}
	}
	update_size_limits();
	on_size_changed();
	if (m_active_tab == pfc_infinite)
		m_active_tab  = TabCtrl_GetCurSel(m_wnd_tabs);

	if (IsWindowVisible(get_wnd()) && m_active_tab < m_panels.get_count() && m_panels[m_active_tab]->m_wnd && !IsWindowVisible(m_panels[m_active_tab]->m_wnd))
	{
		get_host()->on_size_limit_change(get_wnd(), uie::size_limit_all);
		ShowWindow(m_panels[m_active_tab]->m_wnd, SW_SHOWNORMAL);
	}
}

void splitter_window_tabs_impl::destroy_children()
{
	unsigned n, count = m_panels.get_count();
	for (n=0; n<count;n++)
	{
		pfc::refcounted_object_ptr_t<panel> pal = m_panels[n];
		pal->destroy();
	}
	m_active_panels.remove_all();
}

void splitter_window_tabs_impl::insert_panel(unsigned index, const uie::splitter_item_t *  p_item)
{
	if (index <= m_panels.get_count())
	{
		pfc::refcounted_object_ptr_t<panel> temp;
		temp = new panel;
		temp->set_from_splitter_item(p_item);
		m_panels.insert_item(temp, index);

		if (get_wnd())
			refresh_children();
	}
};


void splitter_window_tabs_impl::replace_panel(unsigned index, const uie::splitter_item_t *  p_item)
{
	if (index < m_panels.get_count())
	{
		if (get_wnd())
			m_panels[index]->destroy();

		t_size activeindex = m_active_panels.find_item(m_panels[index]);
		if (activeindex != pfc_infinite)
		{
			m_active_panels.remove_by_idx(activeindex);
			TabCtrl_DeleteItem(m_wnd_tabs, activeindex);
		}

		pfc::refcounted_object_ptr_t<panel> temp;
		temp = new panel;
		temp->set_from_splitter_item(p_item);
		m_panels.replace_item(index, temp);

		if (get_wnd())
			refresh_children();
	}
};

void splitter_window_tabs_impl::remove_panel(unsigned index)
{
	if (index < m_panels.get_count())
	{
		t_size activeindex = m_active_panels.find_item(m_panels[index]);
		if (activeindex != pfc_infinite)
		{
			m_active_panels.remove_by_idx(activeindex);
			TabCtrl_DeleteItem(m_wnd_tabs, activeindex);
		}
		m_panels[index]->destroy();
		m_panels.remove_by_idx(index);
	}
};

void splitter_window_tabs_impl::create_tabs()
{
	g_font = static_api_ptr_t<cui::fonts::manager>()->get_font(g_guid_splitter_tabs);
	RECT rc;
	GetClientRect(get_wnd(), &rc);
	long flags = WS_CHILD |  WS_TABSTOP | TCS_HOTTRACK | TCS_TABS | TCS_MULTILINE | (1 ? WS_VISIBLE : 0); //TCS_MULTILINE hack to prevent BS.
	m_wnd_tabs = CreateWindowEx(0, WC_TABCONTROL, _T("Tab stack"),
		flags, 0, 0, rc.right, rc.bottom,
		get_wnd(), HMENU(2345), core_api::get_my_instance(), NULL);
	SetWindowLongPtr(m_wnd_tabs,GWL_USERDATA,(LPARAM)(this));
	m_tab_proc = (WNDPROC)SetWindowLongPtr(m_wnd_tabs,GWL_WNDPROC,(LPARAM)g_hook_proc);
	//SetWindowTheme(m_wnd_tabs, L"BrowserTab", NULL);
	SendMessage(m_wnd_tabs,WM_SETFONT,(WPARAM)g_font.get(),MAKELPARAM(0,0));
}
void splitter_window_tabs_impl::destroy_tabs()
{
	DestroyWindow(m_wnd_tabs);
	m_wnd_tabs = NULL;
	g_font.release();
}
uie::window_factory<splitter_window_tabs_impl> g_splitter_window_tabs;
service_list_t<splitter_window_tabs_impl> splitter_window_tabs_impl::g_windows;

void splitter_window_tabs_impl::g_on_font_change()
{
	t_size n, count = g_windows.get_count();
	for (n=0; n<count; n++)
	{
		g_windows[n]->on_font_change();
	}
}

void splitter_window_tabs_impl::on_font_change()
{
	if (m_wnd_tabs)
	{
		if (g_font.is_valid())
		{
			uSendMessage(m_wnd_tabs,WM_SETFONT,(WPARAM)0,MAKELPARAM(0,0));
		}

		g_font = static_api_ptr_t<cui::fonts::manager>()->get_font(g_guid_splitter_tabs);

		if (m_wnd_tabs) 
		{
			uSendMessage(m_wnd_tabs,WM_SETFONT,(WPARAM)g_font.get(),MAKELPARAM(1,0));
			update_size_limits();
			get_host()->on_size_limit_change(get_wnd(), uie::size_limit_all);
			on_size_changed();
		}
	}
}
void splitter_window_tabs_impl::on_size_changed(unsigned width, unsigned height)
{
	HDWP dwp = BeginDeferWindowPos(m_active_panels.get_count() +1);
	if (m_wnd_tabs)
		dwp = DeferWindowPos(dwp, m_wnd_tabs, NULL, 0, 0, width, height, SWP_NOZORDER);
	//SetWindowPos(m_wnd_tabs, NULL, 0, 0, width, height, SWP_NOZORDER);
	
	t_size i, count = m_active_panels.get_count();
	RECT rc = {0,0,width,height};
	adjust_rect(FALSE, &rc);
	for (i=0; i<count; i++)
	{
		if (m_active_panels[i]->m_wnd)
			dwp = DeferWindowPos(dwp, m_active_panels[i]->m_wnd, NULL, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
		//SetWindowPos(m_active_panels[i]->m_wnd, NULL, rc.left, rc.top, RECT_CX(rc), RECT_CY(rc), SWP_NOZORDER);
	}
	EndDeferWindowPos(dwp);
}
void splitter_window_tabs_impl::on_size_changed()
{
	RECT rc;
	GetClientRect(get_wnd(), &rc);
	on_size_changed(RECT_CX(rc), RECT_CY(rc));
}
void splitter_window_tabs_impl::on_active_tab_changing(t_size index_from)
{
	if (index_from != pfc_infinite && index_from < m_active_panels.get_count() && m_active_panels[index_from]->m_wnd)
	{
		//if (GetFocus() == m_active_panels[index_from]->m_wnd || IsChild(m_active_panels[index_from]->m_wnd, GetFocus()))
		//{
		//	HWND wnd_root = GetAncestor(get_wnd(), GA_ROOT);
		//	SetFocus(wnd_root);
		//}
		ShowWindow(m_active_panels[index_from]->m_wnd, SW_HIDE);
	}
};
void splitter_window_tabs_impl::on_active_tab_changed(t_size index_to)
{
	if (index_to < m_active_panels.get_count() && m_active_panels[index_to]->m_wnd)
	{
		ShowWindow(m_active_panels[index_to]->m_wnd, SW_SHOW);
		m_active_tab = m_panels.find_item(m_active_panels[index_to]);
	}
	else
		m_active_tab = pfc_infinite;
};
splitter_window_tabs_impl::splitter_window_tabs_impl()
: m_wnd_tabs(NULL), m_active_tab(pfc_infinite), m_tab_proc(NULL), m_mousewheel_delta(NULL)
{};

LRESULT WINAPI splitter_window_tabs_impl::g_hook_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	splitter_window_tabs_impl * p_this;
	LRESULT rv;

	p_this = reinterpret_cast<splitter_window_tabs_impl*>(GetWindowLongPtr(wnd,GWL_USERDATA));

	rv = p_this ? p_this->on_hooked_message(wnd,msg,wp,lp) : DefWindowProc(wnd, msg, wp, lp);;

	return rv;
}

LRESULT WINAPI splitter_window_tabs_impl::on_hooked_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_KEYDOWN:
		{
			if (wp != VK_LEFT && wp != VK_RIGHT && get_host()->get_keyboard_shortcuts_enabled() && g_process_keydown_keyboard_shortcuts(wp)) return 0;
			else if (wp == VK_TAB)
			{
				ui_extension::window::g_on_tab(wnd);
				return 0;
			}
			SendMessage(wnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), NULL);
		}
		break;
	case WM_SYSKEYDOWN:
		if (get_host()->get_keyboard_shortcuts_enabled() && g_process_keydown_keyboard_shortcuts(wp)) return 0;
		break;
	case WM_MOUSEWHEEL:
		if ((GetWindowLongPtr(wnd, GWL_STYLE) & TCS_MULTILINE) == NULL)
		{
			//unsigned scroll_lines = GetNumScrollLines();

			HWND wnd_child = GetWindow(wnd, GW_CHILD);
			WCHAR str_class [129];
			memset(str_class, 0, sizeof(str_class));
			if (wnd_child && RealGetWindowClass (wnd_child, str_class, tabsize(str_class)-1) && !wcscmp(str_class, UPDOWN_CLASS) && IsWindowVisible(wnd_child))
			{

				INT min = NULL, max = NULL, index=NULL;
				BOOL err = FALSE;
				SendMessage(wnd_child, UDM_GETRANGE32, (WPARAM)&min, (LPARAM)&max);
				index = SendMessage(wnd_child, UDM_GETPOS32, (WPARAM)NULL, (LPARAM)&err);

				//if (!err)
				{
					if (max)
					{
						int zDelta = short(HIWORD(wp));

						//int delta = MulDiv(zDelta, scroll_lines, 120);
						m_mousewheel_delta += zDelta;
						int scroll_lines = 1;//GetNumScrollLines();
						//if (scroll_lines == -1)
						//scroll_lines = count;

						if (m_mousewheel_delta*scroll_lines >= WHEEL_DELTA)
						{
							if (index>min)
							{
								SendMessage(wnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, index-1), NULL);
								SendMessage(wnd, WM_HSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), NULL);
								SendMessage(wnd_child, UDM_SETPOS32, NULL, index-1);
							}
							m_mousewheel_delta=0;
						}
						else if (m_mousewheel_delta*scroll_lines <= -WHEEL_DELTA)
						{
							if (index+1<=max)
							{
								SendMessage(wnd, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, index+1), NULL);
								SendMessage(wnd, WM_HSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), NULL);
								SendMessage(wnd_child, UDM_SETPOS32, NULL, index+1);
							}
							m_mousewheel_delta=0;
						}
					}
				}


				return 0;
			}
		}
		break;
	}
	return uCallWindowProc(m_tab_proc,wnd,msg,wp,lp);
}

class font_client_splitter_tabs : public cui::fonts::client
{
public:
	virtual const GUID & get_client_guid() const
	{
		return g_guid_splitter_tabs;
	}
	virtual void get_name (pfc::string_base & p_out) const
	{
		p_out = "Tab Stack";
	}

	virtual cui::fonts::font_type_t get_default_font_type() const
	{
		return cui::fonts::font_type_labels;
	}

	virtual void on_font_changed() const 
	{
		splitter_window_tabs_impl::g_on_font_change();
	}
};

font_client_splitter_tabs::factory<font_client_splitter_tabs> g_font_client_splitter_tabs;
