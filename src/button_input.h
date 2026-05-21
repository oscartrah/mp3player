#pragma once

/**
 * Initialisiert GPIO-Button-Input.
 * Nur relevant auf Boards ohne Touchscreen (LPCXpresso54628).
 * Auf anderen Boards ist die Funktion ein No-Op.
 */
int button_input_init(void);
