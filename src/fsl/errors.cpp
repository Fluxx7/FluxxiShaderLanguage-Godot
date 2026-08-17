#include "errors.h"

void print_fsl_error(const FSLError& error, const FileSourceMap* file_source_ptr) {
    print_error(vformat("File \"%s\" %d,%d: %s", error.file_path, error.start_row + 1, error.start_column, error.error_message));
    String source_segment = "";
    if (file_source_ptr != nullptr && file_source_ptr->has(error.file_path)) {
        const auto& file_sources = *file_source_ptr;
        const LocalVector<String>& file_source = file_sources[error.file_path];
        uint32_t index = error.start_row;
        if (error.start_row == error.end_row) {
            source_segment += file_source[index];
            source_segment += '\n';
            for (uint32_t carat_index = 0; carat_index < file_source[index].length(); carat_index++) {
                if (carat_index == error.start_column) {
                    source_segment += '^';
                    while (++carat_index < file_source[index].length() && carat_index < error.end_column) {
                        source_segment += '~';

                    }
                    break;
                }
                switch (file_source[index][carat_index]) {
                    case '\t':
                        source_segment += '\t';
                        break;
                    default:
                        source_segment += ' ';
                        break;
                }
            }
            index++;
        } else {
            source_segment += file_source[index];

            source_segment += '\n';
            for (uint32_t carat_index = 0; carat_index < file_source[index].length(); carat_index++) {
                if (carat_index == error.start_column) {
                    source_segment += '^';
                    while (carat_index < file_source[index].length()) {
                        source_segment += '~';
                        carat_index++;
                    }
                    break;
                }
                switch (file_source[index][carat_index]) {
                    case '\t':
                        source_segment += '\t';
                        break;
                    default:
                        source_segment += ' ';
                        break;
                }
            }
            source_segment += '\n';
            index++;
            for (; index < error.end_row; index++) {
                source_segment += file_source[index];
                source_segment += '\n';
            }
            source_segment += file_source[index];
            source_segment += '\n';
            for (uint32_t carat_index = 0; carat_index < file_source[index].length(); carat_index++) {
                if (carat_index == error.end_column) {
                    source_segment += '^';
                    break;
                }
                switch (file_source[index][carat_index]) {
                    case '\t':
                        source_segment += '\t';
                        break;
                    default:
                        source_segment += '~';
                        break;
                }
            }
            index++;
        }
        print_error(vformat("Source code:\n%s", source_segment));
    }    
}