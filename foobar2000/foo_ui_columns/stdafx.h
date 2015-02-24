#pragma once

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <vector>
#include <complex>
#include <iostream>

#define OEMRESOURCE

#include "../SDK/foobar2000.h"
#include "../SDK/core_api.h"
/*#define UI_EXTENSION_LIBPNG_SUPPORT_ENABLED*/
#include "uxtheme.h"
#include "Wincodec.h"
#include "../columns_ui-sdk/ui_extension.h"
#include "../ui_helpers/stdafx.h"
#include "../mmh/stdafx.h"
#include "../helpers/helpers.h"
#include "resource.h"
#include "utf8api.h"
#include "helpers.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
/*#include <uxtheme.h>
#include <tmschema.h>*/
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

//#include "..\lpng128\png.h"
#include <io.h>
#include <share.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include "foo_ui_columns.h"

#include "config_items.h"

#include "gdiplus.h"
#include "menu_helpers.h"
#include "prefs.h"
#include "config_vars.h"

#include "callback.h"
#include "common.h"
#include "status_bar.h"
#include "columns_v2.h"
#include "cache.h"
#include "rebar.h"
#include "font_notify.h"
#include "sort.h"
#include "config.h"
#include "splitter.h"
#include "layout.h"

#include "playlist_search.h"
#include "playlist_view.h"
#include "fcs.h"
#include "playlist_manager_utils.h"
#include "playlist_switcher_v2.h"
#include "playlist_tabs.h"
#include "seekbar.h"
#include "vis_gen_host.h"
#include "volume.h"
#include "splitter_tabs.h"
#include "filter.h"
#include "get_msg_hook.h"
#include "setup_dialog.h"
#include "buttons.h"
#include "item_details.h"
#include "mw_drop_target.h"

#include "artwork_helpers.h"
#include "artwork.h"
#include "ng playlist/ng_playlist.h"
#include "item_properties.h"

#include "fcl.h"
#include "menu_items.h"
#include "status_pane.h"
#include "colours_manager_data.h"
#include "fonts_manager_data.h"
#include "config_appearance.h"
#include "tab_colours.h"
#include "tab_fonts.h"
