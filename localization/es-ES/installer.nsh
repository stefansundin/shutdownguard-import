;ShutdownGuard - es-ES localization by Fabrizio Ferri (algernon@gmail.com)
;Do not localize APP_NAME, it will be automatically replaced.
;Keep this file in UTF–16LE
;
;Copyright (C) 2008  Stefan Sundin (recover89@gmail.com)
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.

!if ${L10N_VERSION} == 2

!insertmacro MUI_LANGUAGE "Spanish"
!define LANG ${LANG_SPANISH}

LangString L10N_UPDATE_SECTION    ${LANG} "Comprobar actualizaciones"
LangString L10N_UPDATE_DIALOG     ${LANG} "Una nueva versión está disponible.$\n¿Anular instalación y visitar la página de descarga?"
LangString L10N_RUNNING_INSTALL   ${LANG} "${APP_NAME} se está ejecutando. ¿Desea cerrarlo?"
LangString L10N_RUNNING_UNINSTALL ${LANG} "${APP_NAME} se está ejecutando. ¿Desea cerrarlo?$\nSi elige No, ${APP_NAME} será eliminado por complento en el próximo arranque."
LangString L10N_CLOSING           ${LANG} "Cerrando ${APP_NAME}"
LangString L10N_SHORTCUT          ${LANG} "Acceso directo en el menú Inicio"
LangString L10N_AUTOSTART         ${LANG} "Ejecutar ${APP_NAME} al iniciar Windows"
LangString L10N_AUTOSTART_HIDE    ${LANG} "¿Ocultar el icono en la barra de tareas al iniciar?"

!undef LANG

!else
!warning "Localization out of date!" ;Don't localize this
!endif
