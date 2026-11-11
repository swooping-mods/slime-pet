#ifndef MOON_SLIME_GLYPHS_H
#define MOON_SLIME_GLYPHS_H

/* Original 5x7 label geometry; no Unity, font, allocation or CRT dependency.
 * size is the first line's physical height. Its lower-left origin is (0,0,0),
 * +Y is up and +Z faces the reader. Subsequent lines descend by 8/7*size.
 * Mesh triangles are CCW toward +Z; reverse indices 1/2 at a Unity boundary.
 * Keep SGText in persistent/caller storage: the default worst-case buffer is
 * about 161 KiB. Define SG_MAX_QUADS before inclusion to choose a smaller cap.
 * sg_build returns 1 on success, 0 for invalid input/size or exceeded capacity.
 * Failure leaves the entire output untouched; success owns only count ranges. */
#define SG_MAX_CHARS 64u
#ifndef SG_MAX_QUADS
#define SG_MAX_QUADS (SG_MAX_CHARS*21u)
#endif
#define SG_MAX_VERTICES (SG_MAX_QUADS*4u)
#define SG_MAX_INDICES (SG_MAX_QUADS*6u)

typedef struct { float x,y,z; } SGVec3;
typedef struct {
    unsigned vertex_count,index_count,character_count,line_count;
    float width,height,min_x,min_y,max_x,max_y;
    SGVec3 positions[SG_MAX_VERTICES],normals[SG_MAX_VERTICES];
    int indices[SG_MAX_INDICES];
} SGText;

static inline unsigned sg_finite(float value) {
    union { float value; unsigned bits; } check;
    check.value=value;
    return (check.bits&0x7f800000u)!=0x7f800000u;
}

/* Bit 4 is the leftmost column. Patterns are deliberately monospaced and
 * individually authored for small world labels, including BUY and ZZZ. */
static inline unsigned sg_glyph_row(unsigned char character,unsigned row) {
    static const unsigned char glyphs[95][7] = {
        ['0'-32]={14,17,19,21,25,17,14},
        ['1'-32]={4,12,4,4,4,4,14},
        ['2'-32]={14,17,1,2,4,8,31},
        ['3'-32]={30,1,1,14,1,1,30},
        ['4'-32]={2,6,10,18,31,2,2},
        ['5'-32]={31,16,16,30,1,1,30},
        ['6'-32]={14,16,16,30,17,17,14},
        ['7'-32]={31,1,2,4,8,8,8},
        ['8'-32]={14,17,17,14,17,17,14},
        ['9'-32]={14,17,17,15,1,1,14},
        ['A'-32]={14,17,17,31,17,17,17},
        ['B'-32]={30,17,17,30,17,17,30},
        ['C'-32]={14,17,16,16,16,17,14},
        ['D'-32]={30,17,17,17,17,17,30},
        ['E'-32]={31,16,16,30,16,16,31},
        ['F'-32]={31,16,16,30,16,16,16},
        ['G'-32]={14,17,16,23,17,17,15},
        ['H'-32]={17,17,17,31,17,17,17},
        ['I'-32]={14,4,4,4,4,4,14},
        ['J'-32]={7,2,2,2,2,18,12},
        ['K'-32]={17,18,20,24,20,18,17},
        ['L'-32]={16,16,16,16,16,16,31},
        ['M'-32]={17,27,21,21,17,17,17},
        ['N'-32]={17,25,21,19,17,17,17},
        ['O'-32]={14,17,17,17,17,17,14},
        ['P'-32]={30,17,17,30,16,16,16},
        ['Q'-32]={14,17,17,17,21,18,13},
        ['R'-32]={30,17,17,30,20,18,17},
        ['S'-32]={15,16,16,14,1,1,30},
        ['T'-32]={31,4,4,4,4,4,4},
        ['U'-32]={17,17,17,17,17,17,14},
        ['V'-32]={17,17,17,17,17,10,4},
        ['W'-32]={17,17,17,21,21,21,10},
        ['X'-32]={17,17,10,4,10,17,17},
        ['Y'-32]={17,17,10,4,4,4,4},
        ['Z'-32]={31,1,2,4,8,16,31},
        ['?'-32]={14,17,1,2,4,0,4},
        ['!'-32]={4,4,4,4,4,0,4},
        ['$'-32]={4,15,20,14,5,30,4},
        [':'-32]={0,4,4,0,4,4,0},
        ['/'-32]={1,2,2,4,8,8,16},
        ['+'-32]={0,4,4,31,4,4,0},
        ['-'-32]={0,0,0,31,0,0,0},
        ['.'-32]={0,0,0,0,0,6,6},
        [','-32]={0,0,0,0,6,4,8},
        ['%'-32]={17,2,2,4,8,8,17},
        ['='-32]={0,0,31,0,31,0,0},
        ['('-32]={2,4,8,8,8,4,2},
        [')'-32]={8,4,2,2,2,4,8},
        ['['-32]={14,8,8,8,8,8,14},
        [']'-32]={14,2,2,2,2,2,14},
        ['\''-32]={4,4,8,0,0,0,0},
        ['"'-32]={10,10,20,0,0,0,0},
        ['_'-32]={0,0,0,0,0,0,31},
        ['*'-32]={0,21,14,31,14,21,0},
        ['<'-32]={1,2,4,8,4,2,1},
        ['>'-32]={16,8,4,2,4,8,16},
    };
    unsigned supported;
    if(row>=7u) return 0;
    if(character>='a' && character<='z') character=(unsigned char)(character-32u);
    if(character==' ') return 0;
    supported=(character>='0' && character<='9') || (character>='A' && character<='Z');
    if(!supported) {
        switch(character) {
            case '?': case '!': case '$': case ':': case '/': case '+': case '-':
            case '.': case ',': case '%': case '=': case '(': case ')': case '[':
            case ']': case '\'': case '"': case '_': case '*': case '<': case '>':
                supported=1; break;
            default: break;
        }
    }
    if(!supported) character='?';
    return glyphs[character-32u][row];
}

static inline unsigned sg_build(const char *text,float size,SGText *out) {
    unsigned length=0,quads=0,columns=0,max_columns=0,lines=1;
    unsigned i,row,column,run,mask,vertex,index,line=0;
    float cell,width,height,min_y;
    if(!text || !out || !sg_finite(size) || size<=0.0f) return 0;
    cell=size/7.0f;
    if(cell<=0.0f) return 0;
    /* Count every byte (including newlines), and validate every bound before
     * touching caller memory. Each row needs at most three horizontal spans. */
    while(text[length]) {
        unsigned char character=(unsigned char)text[length];
        if(length>=SG_MAX_CHARS) return 0;
        ++length;
        if(character=='\r') continue;
        if(character=='\n') {
            if(columns>max_columns) max_columns=columns;
            columns=0; ++lines; continue;
        }
        ++columns;
        for(row=0;row<7u;++row) {
            mask=sg_glyph_row(character,row);
            for(column=0;column<5u;++column) {
                if(mask&(16u>>column)) {
                    ++quads;
                    while(column+1u<5u && (mask&(16u>>(column+1u)))) ++column;
                }
            }
        }
        if(quads>SG_MAX_QUADS) return 0;
    }
    if(columns>max_columns) max_columns=columns;
    width=max_columns ? (float)(max_columns*6u-1u)*cell : 0.0f;
    min_y=length ? -(float)((lines-1u)*8u)*cell : 0.0f;
    height=length ? size-min_y : 0.0f;
    if(!sg_finite(width) || !sg_finite(height) || !sg_finite(min_y)) return 0;
    out->vertex_count=out->index_count=0;
    out->character_count=length;
    out->line_count=length ? lines : 0;
    out->width=out->max_x=width;
    out->height=height;
    out->min_x=0.0f; out->min_y=min_y;
    out->max_y=length ? size : 0.0f;
    columns=0;
    for(i=0;i<length;++i) {
        unsigned char character=(unsigned char)text[i];
        if(character=='\r') continue;
        if(character=='\n') { columns=0; ++line; continue; }
        for(row=0;row<7u;++row) {
            mask=sg_glyph_row(character,row);
            for(column=0;column<5u;++column) {
                if(mask&(16u>>column)) {
                    float x0,y0,x1,y1;
                    run=column;
                    while(column+1u<5u && (mask&(16u>>(column+1u)))) ++column;
                    x0=(float)(columns*6u+run)*cell;
                    x1=(float)(columns*6u+column+1u)*cell;
                    y0=size-(float)(line*8u+row+1u)*cell;
                    y1=size-(float)(line*8u+row)*cell;
                    vertex=out->vertex_count; index=out->index_count;
                    out->positions[vertex+0u]=(SGVec3){x0,y0,0};
                    out->positions[vertex+1u]=(SGVec3){x1,y0,0};
                    out->positions[vertex+2u]=(SGVec3){x1,y1,0};
                    out->positions[vertex+3u]=(SGVec3){x0,y1,0};
                    out->normals[vertex+0u]=(SGVec3){0,0,1};
                    out->normals[vertex+1u]=(SGVec3){0,0,1};
                    out->normals[vertex+2u]=(SGVec3){0,0,1};
                    out->normals[vertex+3u]=(SGVec3){0,0,1};
                    out->indices[index+0u]=(int)vertex;
                    out->indices[index+1u]=(int)vertex+1;
                    out->indices[index+2u]=(int)vertex+2;
                    out->indices[index+3u]=(int)vertex;
                    out->indices[index+4u]=(int)vertex+2;
                    out->indices[index+5u]=(int)vertex+3;
                    out->vertex_count+=4u; out->index_count+=6u;
                }
            }
        }
        ++columns;
    }
    return 1;
}

#endif
