namespace hfst {

// *** Wrapper variables for IOString outputs *** //

std::string hfst_xfst_string_one("");
char * get_hfst_xfst_string_one() { return strdup(hfst::hfst_xfst_string_one.c_str()); }
std::string hfst_xfst_string_two("");
char * get_hfst_xfst_string_two() { return strdup(hfst::hfst_xfst_string_two.c_str()); }

int hfst_compile_xfst_to_string_one(hfst::xfst::XfstCompiler & comp, std::string input)
{
        hfst::hfst_xfst_string_one="";
        hfst::hfst_xfst_string_two="";

        std::ostringstream os(std::ostringstream::ate);
        hfst::set_warning_stream(&os);
        comp.set_output_stream(os);
        comp.set_error_stream(os);
        int retval = comp.parse_line(input);
        hfst::hfst_xfst_string_one = os.str();
        hfst::set_warning_stream(&std::cerr);
        return retval;
}

int hfst_compile_xfst(hfst::xfst::XfstCompiler & comp, std::string input, const std::string & output_stream, const std::string & error_stream)
{
        hfst::hfst_xfst_string_one="";
        hfst::hfst_xfst_string_two="";
        std::ostringstream * os1 = NULL;
        std::ostringstream * os2 = NULL;

        if (output_stream == "cout")
          comp.set_output_stream(std::cout);
        else if (output_stream == "cerr")
          comp.set_output_stream(std::cerr);
        else {
          os1 = new std::ostringstream(std::ostringstream::ate);
          comp.set_output_stream(*os1);
        }

        if (error_stream == "cout") {
          comp.set_error_stream(std::cout);
          hfst::set_warning_stream(&std::cout);
        }
        else if (error_stream == "cerr")
          comp.set_error_stream(std::cerr);
        else {
          os2 = new std::ostringstream(std::ostringstream::ate);
          comp.set_error_stream(*os2);
          hfst::set_warning_stream(os2);
        }

        int retval = comp.parse_line(input);
        hfst::set_warning_stream(&std::cerr);
        
        if (output_stream.empty()) {
          hfst::hfst_xfst_string_one = os1->str();
          delete os1;
        }
        if (error_stream.empty()) {
          hfst::hfst_xfst_string_two = os2->str();
          delete os2;
        }

        return retval;
}

}
