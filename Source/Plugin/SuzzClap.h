// SuzzClap.h
// The three entry functions clap-wrapper links into every format.

#pragma once

bool tbsz_entry_init (const char* pluginPath);
void tbsz_entry_deinit();
const void* tbsz_entry_get_factory (const char* factoryId);
