#pragma once

bool GmText_IsParam(uint16_t code);
bool GmText_IsParamSegment(uint16_t code);
bool GmText_IsParamLiteral(uint16_t code);
bool GmText_IsParamNumeric(uint16_t code);

int GmText_ValidateUserMessage(slice_uint16_t text);
int GmText_BuildLiteral(array_uint16_t *buffer, slice_uint16_t text);
