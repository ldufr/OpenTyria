#pragma once

typedef struct Database {
    sqlite3      *conn;
    sqlite3_stmt *stmt_select_session;
    sqlite3_stmt *stmt_select_account;
    sqlite3_stmt *stmt_select_character_and_account;
    sqlite3_stmt *stmt_select_account_characters;
    sqlite3_stmt *stmt_select_character;
    sqlite3_stmt *stmt_select_friendships;
    sqlite3_stmt *stmt_select_character_bags;
    sqlite3_stmt *stmt_insert_character;
    sqlite3_stmt *stmt_delete_character;
    sqlite3_stmt *stmt_create_bag;
    sqlite3_stmt *stmt_create_item;
    sqlite3_stmt *stmt_select_character_items;
    sqlite3_stmt *stmt_update_last_played_character;
} Database;

int Db_Open(Database *result, const char *path);
void Db_Close(Database *database);

int Db_GetSession(Database *database, GmUuid user_id, GmUuid session_id, DbSession *result);
int Db_GetAccount(Database *database, GmUuid account_id, DbAccount *result);
int Db_GetCharacter(Database *database, GmUuid account_id, GmUuid char_id, DbCharacter *result);
int Db_GetCharacters(Database *database, GmUuid account_id, DbCharacterArray *results);
int Db_CharacterBags(Database *database, GmUuid account_id, GmUuid char_id, DbBag *results, size_t count, size_t *returned);

int Db_CreateCharacter(
    Database *database,
    DbCharacter *character,
    size_t n_name, const uint16_t *name);

int Db_DeleteCharacter(Database *database, GmUuid account_id, GmUuid char_id);
int Db_CreateBag(Database *database, DbBag *bag);
int Db_CreateBags(Database *database, DbBag *bags, size_t count);
int Db_CreateItem(Database *database, DbItem *item);
int Db_CreateItems(Database *database, DbItem *items, size_t count);
int Db_UpdateLastPlayedCharacter(Database *database, GmUuid account_id, GmUuid char_id);
