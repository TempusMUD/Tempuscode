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
        Tokenizer(const char *input, char delimiter = ' ') {
            index = 0;
            if (input) {
                length = strlen(input);
                delim = delimiter;
                data = new char[ length + 1 ];
                strcpy( data, input );
            } else {
                length = 0;
                delim = delimiter;
                data = NULL;
            }
        }

        /*
         * Frees the copy of the given input.
         */
        ~Tokenizer() {
            delete data;
            data = NULL;
        }
        /**
         * returns trus if there are tokens left
        **/
        bool hasNext() {
            // obviously the end
            if( index >= length )
                return false;
            // bypass all delimiters
            while( data[index] && data[index] == delim )
                index++;
            return ( index < length );
        }
        /*
         * copies the next token into the given char*
         * returns true if a token is copied, false if not.
         */
        bool next( char *out ) {
            if( index >= length )
                return false;

            // Find the first non delimiter ( token begin )
            int begin;
            while( data[index] && data[index] == delim )
                index++;
            begin = index;

            // Find the next delimiter ( token end )
            while( data[index] && data[index] != delim )
                index++;

            // Copy the token
            for( int i = begin; i < index; i++ ) {
                out[i - begin] = data[i];
            }
            out[index - begin] = '\0';

            // Find the next non delimiter ( next token begin )
            while( data[index] && data[index] == delim )
                index++;
            return true;
        }
        
        /*
         * Copies all remaining data into out
         */
        bool remaining( char *out ) {
            if( index >= length )
                return false;
            strcpy( out, data+index);
            index = length;
            return true;
        }

    private:
        char *data;
        int index;
        int length;
        char delim;
};
