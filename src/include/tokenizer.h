/*
 *  A simple safe rip-off of java.util.StringTokenizer
 */
class Tokenizer {
    public:
        /*
         * Creates a new tokenizer, using input as it's beginning data
         * and delimiter as it's token delimiter.
         *
         * Note:  input is copied into the Tokenizer rather than used
         * directly.
         */
        Tokenizer( const char *input, char delimiter ) {
            index = 0;
            length = strlen(input);
            delim = delimiter;
            data = new char[ length + 1 ];
            strcpy( data, input );
        }

        /*
         * Frees the copy of the given input.
         */
        ~Tokenizer() {
            delete data;
            data = NULL;
        }
        /*
         * copies the next token into the given char*
         * returns true if a token is copied, false if not.
         */
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
