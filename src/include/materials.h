#ifndef _MATERIALS_H_
#define _MATERIALS_H_

//
// File: materials.h                     -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//


#define IS_PAPER_TYPE(obj)     (obj->obj_flags.material >= MAT_PAPER && \
				obj->obj_flags.material < MAT_CLOTH)
#define IS_CLOTH_TYPE(obj)     (obj->obj_flags.material >= MAT_CLOTH && \
			        obj->obj_flags.material < MAT_LEATHER)
#define IS_LEATHER_TYPE(obj)   (obj->obj_flags.material >= MAT_LEATHER && \
			        obj->obj_flags.material < MAT_FLESH)
#define IS_FLESH_TYPE(obj)     (obj->obj_flags.material >= MAT_FLESH && \
			        obj->obj_flags.material < MAT_VEGETABLE)
#define IS_VEGETABLE_TYPE(obj) (obj->obj_flags.material >= MAT_VEGETABLE && \
			        obj->obj_flags.material < MAT_WOOD)
#define IS_WOOD_TYPE(obj)      (obj->obj_flags.material >= MAT_WOOD && \
			        obj->obj_flags.material < MAT_METAL)
#define IS_METAL_TYPE(obj)     (obj->obj_flags.material >= MAT_METAL && \
			        obj->obj_flags.material < MAT_PLASTIC)
#define IS_PLASTIC_TYPE(obj)   (obj->obj_flags.material >= MAT_PLASTIC && \
			        obj->obj_flags.material < MAT_GLASS)
#define IS_GLASS_TYPE(obj)     (obj->obj_flags.material >= MAT_GLASS && \
				obj->obj_flags.material < MAT_STONE)
#define IS_STONE_TYPE(obj)     (obj->obj_flags.material >= MAT_STONE)

#define IS_MAT(obj, mat)       (obj->obj_flags.material == mat)

#define IS_FERROUS(obj)        (IS_MAT(obj, MAT_IRON)   || \
                                IS_MAT(obj, MAT_STEEL)  || \
                                IS_MAT(obj, MAT_BRONZE) || \
                                IS_MAT(obj, MAT_MITHRIL)    || \
                                IS_MAT(obj, MAT_ADAMANTIUM) || \
                                IS_MAT(obj, MAT_METAL) || \
                                IS_MAT(obj, MAT_TIN))

#define IS_BURNABLE_TYPE(obj) (IS_PAPER_TYPE(obj) || \
                               IS_CLOTH_TYPE(obj) || \
                               IS_FLESH_TYPE(obj) || \
                               IS_LEATHER_TYPE(obj) || \
                               IS_VEGETABLE_TYPE(obj) || \
                               IS_WOOD_TYPE(obj))

#define RUSTPILE 15284

#define SOIL_WATER  (1 << 0)
#define SOIL_BLOOD  (1 << 1)
#define SOIL_MUD    (1 << 2)
#define SOIL_SHIT   (1 << 3)
#define SOIL_URINE  (1 << 4)
#define SOIL_MUCUS  (1 << 5)
#define SOIL_SALIVA (1 << 6)
#define SOIL_ACID   (1 << 7)
#define SOIL_OIL    (1 << 8)
#define SOIL_SWEAT  (1 << 9)
#define SOIL_GREASE (1 << 10)
#define SOIL_SOOT   (1 << 11)
#define SOIL_SLIME  (1 << 12)
#define SOIL_STICKY (1 << 13)
#define SOIL_VOMIT  (1 << 14)
#define SOIL_RUST   (1 << 15)
#define SOIL_CHAR   (1 << 16)
#define TOP_SOIL    17

#define MAT_NONE          0
#define MAT_WATER         1
#define MAT_FIRE          2
#define MAT_SHADOW        3
#define MAT_GELATIN       4
#define MAT_LIGHT         5

#define MAT_PAPER         10
#define MAT_PAPYRUS       11
#define MAT_CARDBOARD     12
#define MAT_HEMP          13
#define MAT_PARCHMENT     14

#define MAT_CLOTH         20
#define MAT_SILK          21
#define MAT_COTTON        22
#define MAT_POLYESTER     23
#define MAT_VINYL         24
#define MAT_WOOL          25
#define MAT_SATIN         26
#define MAT_DENIM         27
#define MAT_CARPET        28
#define MAT_VELVET        29
#define MAT_NYLON         30
#define MAT_CANVAS        31
#define MAT_SPONGE        32

#define MAT_LEATHER       40
#define MAT_SUEDE         41
#define MAT_HARD_LEATHER  42
#define MAT_SKIN          43
#define MAT_FUR           44
#define MAT_SCALES        45
#define MAT_HAIR          46
#define MAT_IVORY         57

#define MAT_FLESH         50
#define MAT_BONE          51
#define MAT_TISSUE        52
#define MAT_MEAT_COOKED   53
#define MAT_MEAT_RAW      54
#define MAT_CHEESE        55
#define MAT_EGG           56


#define MAT_VEGETABLE     60
#define MAT_LEAF          61
#define MAT_GRAIN         62
#define MAT_BREAD         63
#define MAT_FRUIT         64
#define MAT_NUT           65
#define MAT_FLOWER        66
#define MAT_FUNGUS        67
#define MAT_SLIME         68

#define MAT_CANDY         70
#define MAT_CHOCOLATE     71
#define MAT_PUDDING       72

#define MAT_WOOD          80
#define MAT_OAK           81
#define MAT_PINE          82
#define MAT_MAPLE         83
#define MAT_BIRCH         84
#define MAT_MAHOGONY      85
#define MAT_TEAK          86
#define MAT_RATTAN        87
#define MAT_EBONY         88
#define MAT_BAMBOO        89

#define MAT_METAL        100
#define MAT_IRON         101
#define MAT_BRONZE       102
#define MAT_STEEL        103
#define MAT_COPPER       104
#define MAT_BRASS        105
#define MAT_SILVER       106
#define MAT_GOLD         107
#define MAT_PLATINUM     108
#define MAT_ELECTRUM     109
#define MAT_LEAD         110
#define MAT_TIN          111
#define MAT_CHROME       112
#define MAT_ALUMINUM     113
#define MAT_SILICON      114
#define MAT_TITANIUM     115
#define MAT_ADAMANTIUM   116
#define MAT_CADMIUM      117
#define MAT_NICKEL       118
#define MAT_MITHRIL      119
#define MAT_PEWTER       120
#define MAT_PLUTONIUM    121
#define MAT_URANIUM      122
#define MAT_RUST         123
#define MAT_ORICHALCUM   124

#define MAT_PLASTIC       140
#define MAT_KEVLAR        141
#define MAT_RUBBER        142
#define MAT_FIBERGLASS    143
#define MAT_ASPHALT       144
#define MAT_CONCRETE      145
#define MAT_WAX           146
#define MAT_PHENOLIC      147
#define MAT_LATEX         148
#define MAT_ENAMEL        149

#define MAT_GLASS         160
#define MAT_CRYSTAL       161
#define MAT_LUCITE        162
#define MAT_PORCELAIN     163
#define MAT_ICE           164
#define MAT_SHELL         165
#define MAT_EARTHENWARE   166
#define MAT_POTTERY       167
#define MAT_CERAMIC       168

#define MAT_STONE            180
#define MAT_AZURITE          181
#define MAT_AGATE            182
#define MAT_AGATE_MOSS       183
#define MAT_AGATE_BANDED     184
#define MAT_AGATE_EYE        185
#define MAT_AGATE_TIGR_EYE   186
#define MAT_QUARTZ           187
#define MAT_QUARTZ_ROSE      188
#define MAT_QUARTZ_SMOKY     189
#define MAT_QUARTZ_2         190
#define MAT_HEMATITE         191
#define MAT_LAPIS_LAZULI     192
#define MAT_MALACHITE        193
#define MAT_OBSIDIAN         194
#define MAT_RHODOCROSITE     195
#define MAT_TIGER_EYE        196
#define MAT_TURQUOISE        197
#define MAT_BLOODSTONE       198
#define MAT_CARNELIAN        199
#define MAT_CHALCEDONY       200
#define MAT_CHYSOPRASE       201
#define MAT_CITRINE          202
#define MAT_JASPER           203
#define MAT_MOONSTONE        204
#define MAT_ONYX             205
#define MAT_ZIRCON           206
#define MAT_AMBER            207
#define MAT_ALEXANDRITE      208
#define MAT_AMETHYST         209
#define MAT_AMETHYST_ORIENTAL 210
#define MAT_AQUAMARINE       211
#define MAT_CHRYSOBERYL      212
#define MAT_CORAL            213
#define MAT_GARNET           214
#define MAT_JADE             215
#define MAT_JET              216
#define MAT_PEARL            217
#define MAT_PERIDOT          218
#define MAT_SPINEL           219
#define MAT_TOPAZ            220
#define MAT_TOPAZ_ORIENTAL   221
#define MAT_TOURMALINE       222
#define MAT_SAPPHIRE         223
#define MAT_SAPPHIRE_BLACK   224
#define MAT_SAPPHIRE_STAR    225
#define MAT_RUBY             226
#define MAT_RUBY_STAR        227
#define MAT_OPAL             228
#define MAT_OPAL_FIRE        229
#define MAT_DIAMOND          230
#define MAT_SANDSTONE        231
#define MAT_MARBLE           232
#define MAT_EMERALD          233
#define MAT_MUD              234
#define MAT_CLAY             235
#define MAT_LABRADORITE      236
#define MAT_IOLITE           237
#define MAT_SPECTROLITE      238
#define MAT_CHAROLITE        239
#define MAT_BASALT           240
#define MAT_ASH              241
#define MAT_INK              242
#define TOP_MATERIAL         243
#endif
