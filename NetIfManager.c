/*
** NetIfManager.c
** MUI 3.8 Application for AmigaOS
** Compiler : SAS/C 6.x
**
** Build:
**   sc NOSTKCHK DATA=NEAR INCLUDEDIR=MUI:Developer/C/Include NetIfManager.c
**   slink FROM LIB:c.o NetIfManager.o TO NetIfManager LIB LIB:amiga.lib NOICONS
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <clib/muimaster_protos.h>
#include <pragmas/muimaster_pragmas.h>
#include <clib/alib_protos.h>

#include <string.h>
#include <stdio.h>

/* MAKE_ID macro */
#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG)(a)<<24|(ULONG)(b)<<16|(ULONG)(c)<<8|(ULONG)(d))
#endif

static const char VersionTag[] =
    "$VER: NetIfManager 1.0 (07.02.2026) Renaud Schweingruber";

struct Library *MUIMasterBase = NULL;
extern struct DosLibrary *DOSBase;
extern struct IntuitionBase *IntuitionBase;

enum
{
    ID_ABOUT = 1,
    ID_QUIT,
    ID_REFRESH,
    ID_STORAGE_LIST,
    ID_DEVS_LIST,
    ID_ADD,
    ID_DELETE
};

#define PATH_STORAGE  "SYS:Storage/NetInterfaces"
#define PATH_DEVS     "DEVS:NetInterfaces"
#define PATH_COPY_SRC "SYS:Storage/NetInterfaces"

/* Globals for GUI objects */
static Object *app;
static Object *window;
static Object *lst_storage;
static Object *lst_devs;
static Object *txt_storage_sel;
static Object *txt_devs_sel;
static Object *btn_refresh;
static Object *btn_quit;
static Object *btn_add;
static Object *btn_delete;

static struct NewMenu MenuData[] =
{
    {NM_TITLE, "Project",    0,   0, 0, 0},
    {NM_ITEM,  "About...",   "?", 0, 0, (APTR)ID_ABOUT},
    {NM_ITEM,  NM_BARLABEL,  0,   0, 0, 0},
    {NM_ITEM,  "Refresh",    "R", 0, 0, (APTR)ID_REFRESH},
    {NM_ITEM,  NM_BARLABEL,  0,   0, 0, 0},
    {NM_ITEM,  "Quit",       "Q", 0, 0, (APTR)ID_QUIT},
    {NM_END,   NULL,          0,   0, 0, 0}
};

/*---------------------------------------------------------------*/
/* GetDeviceName - read device= value from interface file        */
/*---------------------------------------------------------------*/

BOOL GetDeviceName(const char *ifname, const char *dir, char *devname, LONG maxlen)
{
    char filepath[256];
    BPTR fh;
    char line[256];
    BOOL found = FALSE;

    sprintf(filepath, "%s/%s", dir, ifname);

    fh = Open(filepath, MODE_OLDFILE);
    if (!fh) return FALSE;

    while (FGets(fh, line, sizeof(line)))
    {
        char *p;

        /* Look for device= (case insensitive) */
        if (strnicmp(line, "device=", 7) == 0)
        {
            p = line + 7;

            /* Remove trailing whitespace/newline */
            {
                LONG len = strlen(p);
                while (len > 0 && (p[len-1] == '\n' ||
                       p[len-1] == '\r' || p[len-1] == ' '))
                {
                    p[len-1] = '\0';
                    len--;
                }
            }

            strncpy(devname, p, maxlen - 1);
            devname[maxlen - 1] = '\0';
            found = TRUE;
            break;
        }
    }

    Close(fh);
    return found;
}

/*---------------------------------------------------------------*/
/* GetDeviceVersion - get version string via Version command      */
/*---------------------------------------------------------------*/

BOOL GetDeviceVersion(const char *devname, char *verstr, LONG maxlen)
{
    char cmd[256];
    char tmpfile[] = "T:netifmgr_ver.tmp";
    BPTR fh;
    BOOL found = FALSE;

    sprintf(cmd, "Version DEVS:Networks/%s >%s", devname, tmpfile);
    SystemTags(cmd, TAG_DONE);

    fh = Open(tmpfile, MODE_OLDFILE);
    if (fh)
    {
        char line[256];
        if (FGets(fh, line, sizeof(line)))
        {
            /* Remove trailing whitespace/newline */
            LONG len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' ||
                   line[len-1] == '\r' || line[len-1] == ' '))
            {
                line[len-1] = '\0';
                len--;
            }

            strncpy(verstr, line, maxlen - 1);
            verstr[maxlen - 1] = '\0';
            found = TRUE;
        }
        Close(fh);
    }

    DeleteFile(tmpfile);
    return found;
}

/*---------------------------------------------------------------*/
/* ScanDirectory                                                  */
/*---------------------------------------------------------------*/

BOOL ScanDirectory(Object *list, const char *path)
{
    struct FileInfoBlock *fib;
    BPTR lock;
    LONG count = 0;

    DoMethod(list, MUIM_List_Clear);

    lock = Lock((STRPTR)path, ACCESS_READ);
    if (!lock)
    {
        DoMethod(list, MUIM_List_InsertSingle,
            (APTR)"(directory not found)",
            MUIV_List_Insert_Bottom);
        return FALSE;
    }

    fib = AllocDosObject(DOS_FIB, NULL);
    if (!fib)
    {
        UnLock(lock);
        return FALSE;
    }

    if (Examine(lock, fib))
    {
        while (ExNext(lock, fib))
        {
            if (fib->fib_DirEntryType < 0)
            {
                LONG len = strlen(fib->fib_FileName);
                if (len > 5 &&
                    stricmp(fib->fib_FileName + len - 5, ".info") == 0)
                    continue;

                DoMethod(list, MUIM_List_InsertSingle,
                    (APTR)fib->fib_FileName,
                    MUIV_List_Insert_Sorted);
                count++;
            }
        }
    }

    FreeDosObject(DOS_FIB, fib);
    UnLock(lock);

    if (count == 0)
    {
        DoMethod(list, MUIM_List_InsertSingle,
            (APTR)"(empty)",
            MUIV_List_Insert_Bottom);
    }

    return TRUE;
}

/*---------------------------------------------------------------*/
/* CopyInterface - copy file + .info from DH0:Storage to DEVS   */
/*---------------------------------------------------------------*/

void CopyInterface(const char *name)
{
    char src[256];
    char dst[256];
    char src_info[256];
    char dst_info[256];
    char cmd[512];
    LONG rc;

    sprintf(src, "%s/%s", PATH_COPY_SRC, name);
    sprintf(dst, "%s/%s", PATH_DEVS, name);
    sprintf(src_info, "%s/%s.info", PATH_COPY_SRC, name);
    sprintf(dst_info, "%s/%s.info", PATH_DEVS, name);

    /* Copy the interface file */
    sprintf(cmd, "Copy \"%s\" \"%s\" QUIET", src, dst);
    rc = SystemTags(cmd, TAG_DONE);

    if (rc == 0)
    {
        /* Try to copy the icon too, ignore if missing */
        sprintf(cmd, "Copy \"%s\" \"%s\" QUIET", src_info, dst_info);
        SystemTags(cmd, TAG_DONE);
    }
}

/*---------------------------------------------------------------*/
/* DeleteInterface - delete file + .info from DEVS               */
/*---------------------------------------------------------------*/

void DeleteInterface(const char *name)
{
    char path[256];
    char path_info[256];

    sprintf(path, "%s/%s", PATH_DEVS, name);
    sprintf(path_info, "%s/%s.info", PATH_DEVS, name);

    DeleteFile(path);
    DeleteFile(path_info);
}

/*---------------------------------------------------------------*/
/* BuildApp - create MUI objects flat then assemble               */
/*---------------------------------------------------------------*/

BOOL BuildApp(void)
{
    Object *list_obj_s;
    Object *list_obj_d;
    Object *lbl_sel_s;
    Object *lbl_sel_d;
    Object *sel_grp_s;
    Object *sel_grp_d;
    Object *grp_storage;
    Object *grp_devs;
    Object *vbar;
    Object *hgrp_lists;
    Object *title_text;
    Object *hbar1;
    Object *hbar2;
    Object *spacer;
    Object *hgrp_buttons;
    Object *vgrp_main;

    /* List internal objects */
    list_obj_s = ListObject,
        InputListFrame,
        MUIA_List_Title, TRUE,
        MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
        MUIA_List_DestructHook, MUIV_List_DestructHook_String,
    End;
    if (!list_obj_s) return FALSE;

    list_obj_d = ListObject,
        InputListFrame,
        MUIA_List_Title, TRUE,
        MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
        MUIA_List_DestructHook, MUIV_List_DestructHook_String,
    End;
    if (!list_obj_d) return FALSE;

    /* Listviews */
    lst_storage = ListviewObject,
        MUIA_Listview_List, list_obj_s,
    End;
    if (!lst_storage) return FALSE;

    lst_devs = ListviewObject,
        MUIA_Listview_List, list_obj_d,
    End;
    if (!lst_devs) return FALSE;

    /* Selection labels */
    lbl_sel_s = TextObject,
        MUIA_Text_Contents, "\33bDevice:",
        MUIA_Text_SetMin, FALSE,
    End;
    if (!lbl_sel_s) return FALSE;

    lbl_sel_d = TextObject,
        MUIA_Text_Contents, "\33bDevice:",
        MUIA_Text_SetMin, FALSE,
    End;
    if (!lbl_sel_d) return FALSE;

    /* Selection text fields */
    txt_storage_sel = TextObject,
        TextFrame,
        MUIA_Text_Contents, "\33c(no selection)",
        MUIA_Text_SetMin, FALSE,
    End;
    if (!txt_storage_sel) return FALSE;

    txt_devs_sel = TextObject,
        TextFrame,
        MUIA_Text_Contents, "\33c(no selection)",
        MUIA_Text_SetMin, FALSE,
    End;
    if (!txt_devs_sel) return FALSE;

    /* Selection sub-groups */
    sel_grp_s = VGroup,
        Child, lbl_sel_s,
        Child, txt_storage_sel,
    End;
    if (!sel_grp_s) return FALSE;

    sel_grp_d = VGroup,
        Child, lbl_sel_d,
        Child, txt_devs_sel,
    End;
    if (!sel_grp_d) return FALSE;

    /* Action buttons */
    btn_add = SimpleButton("_Add >>");
    if (!btn_add) return FALSE;

    btn_delete = SimpleButton("<< _Remove");
    if (!btn_delete) return FALSE;

    /* Left panel */
    grp_storage = VGroup,
        GroupFrameT("SYS:Storage/NetInterfaces"),
        Child, lst_storage,
        Child, sel_grp_s,
        Child, btn_add,
    End;
    if (!grp_storage) return FALSE;

    /* Right panel */
    grp_devs = VGroup,
        GroupFrameT("DEVS:NetInterfaces"),
        Child, lst_devs,
        Child, sel_grp_d,
        Child, btn_delete,
    End;
    if (!grp_devs) return FALSE;

    /* Vertical bar */
    vbar = RectangleObject,
        MUIA_Rectangle_VBar, TRUE,
        MUIA_FixWidth, 8,
    End;
    if (!vbar) return FALSE;

    /* Both panels side by side */
    hgrp_lists = HGroup,
        Child, grp_storage,
        Child, vbar,
        Child, grp_devs,
    End;
    if (!hgrp_lists) return FALSE;

    /* Title */
    title_text = TextObject,
        MUIA_Text_Contents, "\33b\33cNetwork Interface Manager",
        MUIA_Text_SetMin, FALSE,
        MUIA_Font, MUIV_Font_Big,
    End;
    if (!title_text) return FALSE;

    /* Separators */
    hbar1 = RectangleObject,
        MUIA_Rectangle_HBar, TRUE,
        MUIA_FixHeight, 8,
    End;
    if (!hbar1) return FALSE;

    hbar2 = RectangleObject,
        MUIA_Rectangle_HBar, TRUE,
        MUIA_FixHeight, 4,
    End;
    if (!hbar2) return FALSE;

    /* Buttons */
    btn_refresh = SimpleButton("_Refresh");
    if (!btn_refresh) return FALSE;

    btn_quit = SimpleButton("_Quit");
    if (!btn_quit) return FALSE;

    spacer = HSpace(0);
    if (!spacer) return FALSE;

    hgrp_buttons = HGroup,
        MUIA_Group_SameSize, TRUE,
        Child, btn_refresh,
        Child, spacer,
        Child, btn_quit,
    End;
    if (!hgrp_buttons) return FALSE;

    /* Main vertical group */
    vgrp_main = VGroup,
        Child, title_text,
        Child, hbar1,
        Child, hgrp_lists,
        Child, hbar2,
        Child, hgrp_buttons,
    End;
    if (!vgrp_main) return FALSE;

    /* Window */
    window = WindowObject,
        MUIA_Window_Title, "NetIfManager - Network Interfaces",
        MUIA_Window_ID, MAKE_ID('N','I','F','M'),
        WindowContents, vgrp_main,
    End;
    if (!window) return FALSE;

    /* Application */
    app = ApplicationObject,
        MUIA_Application_Title, "NetIfManager",
        MUIA_Application_Version, (ULONG)VersionTag + 6,
        MUIA_Application_Copyright, "(c)2026",
        MUIA_Application_Author, "Renaud Schweingruber",
        MUIA_Application_Description, "Network Interface Manager",
        MUIA_Application_Base, "NETIFMGR",
        MUIA_Application_Menu, MenuData,
        SubWindow, window,
    End;

    return (BOOL)(app != NULL);
}

/*---------------------------------------------------------------*/
/* Main                                                           */
/*---------------------------------------------------------------*/

int main(int argc, char **argv)
{
    BOOL running = TRUE;
    ULONG signals;
    ULONG open;

    MUIMasterBase = OpenLibrary(MUIMASTER_NAME, 19);
    if (!MUIMasterBase)
    {
        PutStr("Cannot open muimaster.library v19\n");
        return RETURN_FAIL;
    }

    if (!BuildApp())
    {
        PutStr("Cannot create application.\n");
        if (app) MUI_DisposeObject(app);
        CloseLibrary(MUIMasterBase);
        return RETURN_FAIL;
    }

    /* Notifications */
    DoMethod(window, MUIM_Notify,
        MUIA_Window_CloseRequest, TRUE,
        app, 2,
        MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit);

    DoMethod(lst_storage, MUIM_Notify,
        MUIA_List_Active, MUIV_EveryTime,
        app, 2,
        MUIM_Application_ReturnID, ID_STORAGE_LIST);

    DoMethod(lst_devs, MUIM_Notify,
        MUIA_List_Active, MUIV_EveryTime,
        app, 2,
        MUIM_Application_ReturnID, ID_DEVS_LIST);

    DoMethod(app, MUIM_Notify,
        MUIA_Application_MenuAction, ID_ABOUT,
        app, 2,
        MUIM_Application_ReturnID, ID_ABOUT);

    DoMethod(app, MUIM_Notify,
        MUIA_Application_MenuAction, ID_REFRESH,
        app, 2,
        MUIM_Application_ReturnID, ID_REFRESH);

    DoMethod(app, MUIM_Notify,
        MUIA_Application_MenuAction, ID_QUIT,
        app, 2,
        MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit);

    DoMethod(btn_refresh, MUIM_Notify,
        MUIA_Pressed, FALSE,
        app, 2,
        MUIM_Application_ReturnID, ID_REFRESH);

    DoMethod(btn_quit, MUIM_Notify,
        MUIA_Pressed, FALSE,
        app, 2,
        MUIM_Application_ReturnID,
        MUIV_Application_ReturnID_Quit);

    DoMethod(btn_add, MUIM_Notify,
        MUIA_Pressed, FALSE,
        app, 2,
        MUIM_Application_ReturnID, ID_ADD);

    DoMethod(btn_delete, MUIM_Notify,
        MUIA_Pressed, FALSE,
        app, 2,
        MUIM_Application_ReturnID, ID_DELETE);

    /* Double-click on left list = Add */
    DoMethod(lst_storage, MUIM_Notify,
        MUIA_Listview_DoubleClick, TRUE,
        app, 2,
        MUIM_Application_ReturnID, ID_ADD);

    /* Double-click on right list = Delete */
    DoMethod(lst_devs, MUIM_Notify,
        MUIA_Listview_DoubleClick, TRUE,
        app, 2,
        MUIM_Application_ReturnID, ID_DELETE);

    /* Initial scan */
    ScanDirectory(lst_storage, PATH_STORAGE);
    ScanDirectory(lst_devs, PATH_DEVS);

    /* Open window */
    set(window, MUIA_Window_Open, TRUE);

    get(window, MUIA_Window_Open, &open);
    if (!open)
    {
        PutStr("Cannot open window.\n");
        MUI_DisposeObject(app);
        CloseLibrary(MUIMasterBase);
        return RETURN_FAIL;
    }

    /* Main loop */
    while (running)
    {
        ULONG id;

        id = DoMethod(app, MUIM_Application_NewInput, &signals);

        switch (id)
        {
            case MUIV_Application_ReturnID_Quit:
                running = FALSE;
                break;

            case ID_ABOUT:
            {
                Object *aboutwin;
                Object *abouttext;
                Object *btnok;
                Object *btngithub;
                Object *aboutgrp;
                Object *aboutbtns;
                BOOL aboutopen = TRUE;
                ULONG aboutsigs;

                abouttext = TextObject,
                    MUIA_Text_Contents,
                        "\33c\33bNetIfManager 1.0\33n\n"
                        "Network Interface Manager\n\n"
                        "Renaud Schweingruber\n"
                        "renaud.schweingruber@protonmail.com\n\n"
                        "https://github.com/TuKo1982/NetIfManager",
                    MUIA_Text_SetMin, TRUE,
                End;

                btnok = SimpleButton("_OK");
                btngithub = SimpleButton("_Visit GitHub");

                aboutbtns = HGroup,
                    MUIA_Group_SameSize, TRUE,
                    Child, btngithub,
                    Child, btnok,
                End;

                aboutgrp = VGroup,
                    Child, abouttext,
                    Child, aboutbtns,
                End;

                aboutwin = WindowObject,
                    MUIA_Window_Title, "About NetIfManager",
                    MUIA_Window_ID, MAKE_ID('A','B','O','T'),
                    MUIA_Window_RefWindow, window,
                    WindowContents, aboutgrp,
                End;

                if (aboutwin)
                {
                    DoMethod(app, OM_ADDMEMBER, aboutwin);

                    DoMethod(aboutwin, MUIM_Notify,
                        MUIA_Window_CloseRequest, TRUE,
                        app, 2,
                        MUIM_Application_ReturnID, 0xAB01);

                    DoMethod(btnok, MUIM_Notify,
                        MUIA_Pressed, FALSE,
                        app, 2,
                        MUIM_Application_ReturnID, 0xAB01);

                    DoMethod(btngithub, MUIM_Notify,
                        MUIA_Pressed, FALSE,
                        app, 2,
                        MUIM_Application_ReturnID, 0xAB02);

                    set(aboutwin, MUIA_Window_Open, TRUE);

                    while (aboutopen)
                    {
                        ULONG aid;
                        aid = DoMethod(app, MUIM_Application_NewInput, &aboutsigs);

                        switch (aid)
                        {
                            case 0xAB01:
                                aboutopen = FALSE;
                                break;

                            case 0xAB02:
                                Execute((STRPTR)"C:OpenURL https://github.com/TuKo1982/NetIfManager", 0, 0);
                                aboutopen = FALSE;
                                break;

                            case MUIV_Application_ReturnID_Quit:
                                aboutopen = FALSE;
                                running = FALSE;
                                break;
                        }

                        if (aboutopen && aboutsigs)
                            Wait(aboutsigs);
                    }

                    set(aboutwin, MUIA_Window_Open, FALSE);
                    DoMethod(app, OM_REMMEMBER, aboutwin);
                    MUI_DisposeObject(aboutwin);
                }
                break;
            }

            case ID_REFRESH:
                ScanDirectory(lst_storage, PATH_STORAGE);
                ScanDirectory(lst_devs, PATH_DEVS);
                set(txt_storage_sel, MUIA_Text_Contents,
                    "\33c(no selection)");
                set(txt_devs_sel, MUIA_Text_Contents,
                    "\33c(no selection)");
                break;

            case ID_STORAGE_LIST:
            {
                STRPTR entry = NULL;
                DoMethod(lst_storage, MUIM_List_GetEntry,
                    MUIV_List_GetEntry_Active, &entry);
                if (entry)
                {
                    char buf[512];
                    char devname[128];
                    char verstr[256];

                    if (GetDeviceName(entry, PATH_STORAGE, devname, sizeof(devname)))
                    {
                        if (GetDeviceVersion(devname, verstr, sizeof(verstr)))
                            sprintf(buf, "\33c\33b%s", verstr);
                        else
                            sprintf(buf, "\33c\33b%s", devname);
                    }
                    else
                        sprintf(buf, "\33c\33b%s", entry);

                    set(txt_storage_sel,
                        MUIA_Text_Contents, buf);
                }
                break;
            }

            case ID_DEVS_LIST:
            {
                STRPTR entry = NULL;
                DoMethod(lst_devs, MUIM_List_GetEntry,
                    MUIV_List_GetEntry_Active, &entry);
                if (entry)
                {
                    char buf[512];
                    char devname[128];
                    char verstr[256];

                    if (GetDeviceName(entry, PATH_DEVS, devname, sizeof(devname)))
                    {
                        if (GetDeviceVersion(devname, verstr, sizeof(verstr)))
                            sprintf(buf, "\33c\33b%s", verstr);
                        else
                            sprintf(buf, "\33c\33b%s", devname);
                    }
                    else
                        sprintf(buf, "\33c\33b%s", entry);

                    set(txt_devs_sel,
                        MUIA_Text_Contents, buf);
                }
                break;
            }

            case ID_ADD:
            {
                STRPTR entry = NULL;
                DoMethod(lst_storage, MUIM_List_GetEntry,
                    MUIV_List_GetEntry_Active, &entry);
                if (entry && strcmp(entry, "(empty)") != 0
                          && strcmp(entry, "(directory not found)") != 0)
                {
                    CopyInterface(entry);
                    ScanDirectory(lst_devs, PATH_DEVS);
                    set(txt_devs_sel, MUIA_Text_Contents,
                        "\33c(no selection)");
                }
                break;
            }

            case ID_DELETE:
            {
                STRPTR entry = NULL;
                DoMethod(lst_devs, MUIM_List_GetEntry,
                    MUIV_List_GetEntry_Active, &entry);
                if (entry && strcmp(entry, "(empty)") != 0
                          && strcmp(entry, "(directory not found)") != 0)
                {
                    DeleteInterface(entry);
                    ScanDirectory(lst_devs, PATH_DEVS);
                    set(txt_devs_sel, MUIA_Text_Contents,
                        "\33c(no selection)");
                }
                break;
            }
        }

        if (running && signals)
            Wait(signals);
    }

    /* Cleanup */
    set(window, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);
    CloseLibrary(MUIMasterBase);

    return RETURN_OK;
}
