#pragma once

CharacterSettings CharacterSettings_FromDbCharacter(DbCharacter *character)
{
    CharacterSettings settings = {0};
    settings.version = CHARACTER_SETTINGS_VERSION;
    settings.last_outpost = character->last_outpost;
    settings.last_time_played = 0;
    settings.appearance.sex = character->sex;
    settings.appearance.height = character->height;
    settings.appearance.skin_color = character->skin_color;
    settings.appearance.hair_color = character->hair_color;
    settings.appearance.face_style = character->face_style;
    settings.appearance.primary_profession = character->primary_profession;
    settings.appearance.hair_style = character->hair_style;
    settings.appearance.race = character->race;
    settings.last_guild_hall_id = character->last_guild_hall_id;
    settings.campaign = character->campaign;
    settings.level = character->level;
    settings.is_pvp = character->is_pvp;
    settings.secondary_profession = character->secondary_profession;
    settings.helm_status = character->helm_status;

    if (character->file_id_body) {
        settings.pieces[settings.number_of_pieces].file_id = character->file_id_body;
        settings.pieces[settings.number_of_pieces].col1 = DyeColor_First(character->colors_body);
        settings.pieces[settings.number_of_pieces].col2 = DyeColor_Second(character->colors_body);
        settings.pieces[settings.number_of_pieces].col3 = DyeColor_Third(character->colors_body);
        settings.pieces[settings.number_of_pieces].col4 = DyeColor_Fourth(character->colors_body);
        ++settings.number_of_pieces;
    }

    if (character->file_id_legs) {
        settings.pieces[settings.number_of_pieces].file_id = character->file_id_legs;
        settings.pieces[settings.number_of_pieces].col1 = DyeColor_First(character->colors_legs);
        settings.pieces[settings.number_of_pieces].col2 = DyeColor_Second(character->colors_legs);
        settings.pieces[settings.number_of_pieces].col3 = DyeColor_Third(character->colors_legs);
        settings.pieces[settings.number_of_pieces].col4 = DyeColor_Fourth(character->colors_legs);
        ++settings.number_of_pieces;
    }

    if (character->file_id_head) {
        settings.pieces[settings.number_of_pieces].file_id = character->file_id_head;
        settings.pieces[settings.number_of_pieces].col1 = DyeColor_First(character->colors_head);
        settings.pieces[settings.number_of_pieces].col2 = DyeColor_Second(character->colors_head);
        settings.pieces[settings.number_of_pieces].col3 = DyeColor_Third(character->colors_head);
        settings.pieces[settings.number_of_pieces].col4 = DyeColor_Fourth(character->colors_head);
        ++settings.number_of_pieces;
    }

    if (character->file_id_boots) {
        settings.pieces[settings.number_of_pieces].file_id = character->file_id_boots;
        settings.pieces[settings.number_of_pieces].col1 = DyeColor_First(character->colors_boots);
        settings.pieces[settings.number_of_pieces].col2 = DyeColor_Second(character->colors_boots);
        settings.pieces[settings.number_of_pieces].col3 = DyeColor_Third(character->colors_boots);
        settings.pieces[settings.number_of_pieces].col4 = DyeColor_Fourth(character->colors_boots);
        ++settings.number_of_pieces;
    }

    if (character->file_id_gloves) {
        settings.pieces[settings.number_of_pieces].file_id = character->file_id_gloves;
        settings.pieces[settings.number_of_pieces].col1 = DyeColor_First(character->colors_gloves);
        settings.pieces[settings.number_of_pieces].col2 = DyeColor_Second(character->colors_gloves);
        settings.pieces[settings.number_of_pieces].col3 = DyeColor_Third(character->colors_gloves);
        settings.pieces[settings.number_of_pieces].col4 = DyeColor_Fourth(character->colors_gloves);
        ++settings.number_of_pieces;
    }

    return settings;
}
