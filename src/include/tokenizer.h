class Tokenizer {
    public:
        Tokenizer( const char *input, char delimiter ) {
            index = 0;
            length = strlen(input);
            delim = delimiter;
            data = new char[ length + 1 ];
            strcpy( data, input );
        }

        ~Tokenizer() {
            delete data;
            data = NULL;
        }

        bool next( char *out ) {
            if( index >= length )
                return false;
            int begin;
            while( data[index] && data[index] == delim )
                index++;
            begin = index;
            while( data[index] && data[index] != delim )
                index++;
            for( int i = begin; i < index; i++ ) {
                out[i - begin] = data[i];
            }
            out[index - begin] = '\0';
        }

    private:
        char *data;
        int index;
        int length;
        char delim;
};
