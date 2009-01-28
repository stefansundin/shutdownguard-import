;ShutdownGuard - nn-NO lokalisering av Jon Stødle (jonstodle@gmail.com=
;Do not localize APP_NAME, it will be automatically replaced.
;Keep this file in UTF–16LE
;
;Copyright (C) 2009  Stefan Sundin (recover89@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

!if ${L10N_VERSION} == 2

!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!define LANG ${LANG_NORWEGIANNYNORSK}

LangString L10N_UPDATE_SECTION    ${LANG} "Sjekk etter oppdateringar"
LangString L10N_UPDATE_DIALOG     ${LANG} "Ein ny versjon er tilgjengeleg.$\nAvbryte installasjon og gå til vevsida?"
LangString L10N_RUNNING_INSTALL   ${LANG} "${APP_NAME} køyrer. Lukke?"
LangString L10N_RUNNING_UNINSTALL ${LANG} "${APP_NAME} køyrer. Lukke?$\nOm du vel Nei, ${APP_NAME} vil verte fullstendig fjerne ved neste omstart."
LangString L10N_CLOSING           ${LANG} "Lukker ${APP_NAME}"
LangString L10N_SHORTCUT          ${LANG} "Startmeny-snarveg"
LangString L10N_AUTOSTART         ${LANG} "Autostart"
LangString L10N_AUTOSTART_HIDE    ${LANG} "Gøym ikon"

!undef LANG

!else
!warning "Localization out of date!" ;Don't localize this
!endif
