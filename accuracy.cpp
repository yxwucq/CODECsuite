//
// Created by Ruolin Liu on 3/3/20.
//
//
// Created by Ruolin Liu on 2/18/20.
//

#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>
#include <chrono>
#include <ctime>

//#include "SeqLib/BWAWrapper.h"
#include "SeqLib/BamWriter.h"
#ifdef BGZF_MAX_BLOCK_SIZE
#pragma push_macro("BGZF_MAX_BLOCK_SIZE")
#undef BGZF_MAX_BLOCK_SIZE
#define BGZF_MAX_BLOCK_SIZE_BAK
#endif

#ifdef BGZF_BLOCK_SIZE
#pragma push_macro("BGZF_BLOCK_SIZE")
#undef BGZF_BLOCK_SIZE
#define BGZF_BLOCK_SIZE_BAK
#endif

#include "InsertSeqFactory.h"
#include "ReadVCF.h"
#include "Variant.h"
#include "Alignment.h"
#include "StringUtils.h"
#include "MAF.h"
#include "TargetLayout.h"
#include "MutCounter.h"
#include "Algo.h"
#include "pileup.h"

#define OPT_QSCORE_PROF   261
#define OPT_READ_LEVEL_STAT   262
#define OPT_CYCLE_LEVEL_STAT   263
#define OPT_MAX_N_RATE_T2G   264
using std::string;
using std::vector;
int MYINT_MAX = std::numeric_limits<int>::max();

struct AccuOptions {
  string bam;
  string accuracy_stat = "accuracy_stat.txt";
  string error_prof_out = "error_prof_out.txt";
  int mapq = 10;
  int bqual_min = 0;
  bool load_supplementary = false;
  bool load_secondary = false;
  bool load_unpair = false;
  bool load_duplicate = false;
  bool all_mutant_frags = false;
  bool filter_5endclip = false;
  bool allow_indel_near_snv = false;
  int max_N_filter = MYINT_MAX;
  int max_snv_filter = MYINT_MAX;
  int min_fraglen = 0;
  int max_fraglen = MYINT_MAX;
  int verbose = 0;
  int clustered_mut_cutoff = MYINT_MAX;
  string germline_bam;
  int germline_minbq = 10;
  int germline_minmapq = 20;
  int germline_mindepth = 5;
  double max_mismatch_frac = 1.0;
  float max_N_frac = 1.0;
  float max_N_frac_T2G = 1.0;
  float min_passQ_frac = 0;
  vector<string> vcfs;
  string sample = "";
  string reference;
  int fragend_dist_filter = 0;
  string known_var_out;
  string context_count = "context_count.txt";
  bool detail_qscore_prof = false;
  string read_level_stat;
  string cycle_level_stat;
  int pair_min_overlap = 0;
  bool count_read = false;
  string maf_file;
  string bed_file;
  string trim_bam = "trimmed.bam";
};


static struct option  accuracy_long_options[] = {
    {"bam",                      required_argument,      0,        'b'},
    {"normal_bam",               required_argument,      0,        'n'},
    {"bed",                      required_argument,      0,        'L'},
    {"accuracy_stat",            required_argument,      0,        'a'},
    {"load_unpair",              no_argument,            0,        'u'},
    {"load_supplementary",       no_argument,            0,        'S'},
    {"load_secondary",           no_argument,            0,        '2'},
    {"load_duplicate",           no_argument,            0,        'D'},
    {"mapq",                     required_argument ,     0,        'm'},
    {"vcfs",                     required_argument ,     0,        'V'},
    {"maf",                      required_argument ,     0,        'M'},
    {"sample",                   required_argument,      0,        's'},
    {"reference",                required_argument,      0,        'r'},
    {"error_prof_out",           required_argument,      0,        'e'},
    {"bqual_min",                required_argument,      0,        'q'},
    {"min_fraglen",              required_argument,      0,        'g'},
    {"max_fraglen",              required_argument,      0,        'G'},
    {"max_mismatch_frac",        required_argument,      0,        'F'},
    {"filter_5endclip",          no_argument,            0,        '5'},
    {"allow_indel_near_snv",     no_argument,            0,        'I'},
    {"max_N_frac",               required_argument,      0,        'N'},
    {"max_N_frac_T2G",           required_argument,      0,        OPT_MAX_N_RATE_T2G},
    {"min_passQ_frac",           required_argument,      0,        'Q'},
    {"known_var_out",            required_argument,      0,        'k'},
    {"context_count",            required_argument,      0,        'C'},
    {"pair_min_overlap",         required_argument,      0,        'p'},
    {"count_read",               no_argument,            0,        'R'},
    {"fragend_dist_filter",      required_argument,      0,        'd'},
    {"max_N_filter",             required_argument,      0,        'y'},
    {"max_snv_filter",           required_argument,      0,        'x'},
    {"clustered_mut_cutoff",     required_argument,      0,        'c'},
    {"verbose",                  required_argument,      0,        'v'},
    {"all_mutant_frags",         no_argument,             0,        'A'},
    {"detail_qscore_prof",       no_argument,            0,        OPT_QSCORE_PROF},
    {"read_level_stat",          required_argument,      0,        OPT_READ_LEVEL_STAT},
    {"cycle_level_stat",         required_argument,      0,        OPT_CYCLE_LEVEL_STAT},
    {0,0,0,0}
};
const char* accuracy_short_options = "b:a:m:v:S2us:r:e:q:k:RM:p:d:n:x:V:L:DAC:F:N:5Q:g:G:Ic:";

void accuracy_print_help()
{
  std::cerr<< "---------------------------------------------------\n";
  std::cerr<< "Usage: codec accuracy [options]\n";
  std::cerr<< "General Options:\n";
  std::cerr<< "-v/--verbose,                          [default 0]\n";
  std::cerr<< "-b/--bam,                              input bam\n";
  std::cerr<< "-n/--normal_bam,                       input normal bam [optional]\n";
  std::cerr<< "-L/--bed,                              targeted region\n";
  std::cerr<< "-m/--mapq,                             min mapping quality [10].\n";

  /*Hide options*/
//  std::cerr<< "-S/--load_supplementary,               include supplementary alignment [false].\n";
//  std::cerr<< "-2/--load_secondary,                   include secondary alignment [false].\n";
  std::cerr<< "-u/--load_unpair,                      include unpaired alignment [false].\n";
  std::cerr<< "-D/--load_duplicate,                   include alignment marked as duplicates [false].\n";
  std::cerr<< "-V/--vcfs,                             comma separated VCF file(s) for germline variants or whitelist variants[null].\n";
  std::cerr<< "-M/--maf,                              MAF file for somatic variants [null].\n";
  std::cerr<< "-s/--sample,                           sample from the VCF file [null].\n";
  std::cerr<< "-r/--reference,                        reference sequence in fasta format [null].\n";
  std::cerr<< "-a/--accuracy_stat,                    output reporting accuracy for each alignment [accuracy_stat.txt].\n";
  std::cerr<< "-e/--error_prof_out,                   Error profile output in plain txt format [error_prof_out.txt].\n";
  std::cerr<< "-k/--known_var_out,                    Output for known var. [known_var_out.txt].\n";
  std::cerr<< "-C/--context_count,                    Output for trinucleotide and dinucleotide context context in the reference. [context_count.txt].\n";
  //std::cerr<< "--qscore_prof,                         Output qscore prof. First column is qscore cutoff; second column is number of bases in denominator\n";
  std::cerr<< "--detail_qscore_prof,                  Output finer scale qscore cutoffs, error rates profile. The default is only q0, q30 [false]. \n";
  std::cerr<< "--read_level_stat,                     Output read level error metrics.\n";
  std::cerr<< "--cycle_level_stat,                    Output cycle level error metrics.\n";
  std::cerr<< "\nFiltering Options:\n";
  std::cerr<< "-q/--bqual_min,                        Skip bases if min(q1, q2) < this when calculating error rate. q1, q2 are baseQ from R1 and R2 respectively [0].\n";
  std::cerr<< "-Q/--min_passQ_frac,                   Filter out a read if the fraction of bases passing quality threshold (together with -q) is less than this number [0].\n";
  std::cerr<< "-x/--max_snv_filter,                   Skip a read if the number of mismatch bases is larger than this value [INT_MAX].\n";
  std::cerr<< "-y/--max_N_filter,                     Skip a read if its num of N bases is larger than this value [INT_MAX].\n";
  std::cerr<< "-d/--fragend_dist_filter,              Consider a variant if its distance to the fragment end is at least this value [0].\n";
  std::cerr<< "-F/--max_mismatch_frac,                Filter out a read if its mismatch fraction is larger than this value  [1.0].\n";
  std::cerr<< "-N/--max_N_frac,                       Filter out a read if its fraction of of N larger than this value  [1.0].\n";
  std::cerr<< "--max_N_frac_T2G,                      Filter out T>G SNV if in its reads, the fraction of of N larger than this value  [1.0].\n";
  std::cerr<< "-5/--filter_5endclip,                  Filter out a read if it has 5'end soft clipping [false].\n";
  std::cerr<< "-I/--allow_indel_near_snv,             allow SNV to pass filter if a indel is found in the same read [false].\n";
  std::cerr<< "-g/--min_fraglen,                      Filter out a read if its fragment length is less than this value [0].\n";
  std::cerr<< "-G/--max_fraglen,                      Filter out a read if its fragment length is larger than this value [INT_MAX].\n";
  std::cerr<< "-p/--pair_min_overlap,                 When using selector, the minimum overlap between the two ends of the pair. -1 for complete overlap, 0 no overlap required [0].\n";
  std::cerr<< "-R/--count_read,                       When this is true, count #valid bases independently for R1 and R2, including overhang (non overlapping) region of a read pair. [false].\n";
  std::cerr<< "-c/--clustered_mut_cutoff,             Filter out a read if num. mutations are clustered together. [INT_MAX].\n";
  //std::cerr<< "-A/--all_mutant_frags,                 Output all mutant fragments even if not pass failters. Currently only works for known vars [false].\n";
}

int accuracy_parse_options(int argc, char* argv[], AccuOptions& opt) {
  int option_index;
  int next_option = 0;
  do {
    next_option = getopt_long(argc, argv, accuracy_short_options, accuracy_long_options, &option_index);
    switch (next_option) {
      case -1:break;
      case 'v':
        opt.verbose = atoi(optarg);
        break;
      case 'b':
        opt.bam = optarg;
        break;
      case 'n':
        opt.germline_bam = optarg;
        break;
      case 'L':
        opt.bed_file = optarg;
        break;
      case 'a':
        opt.accuracy_stat = optarg;
        break;
      case 'm':
        opt.mapq = atoi(optarg);
        break;
      case 'q':
        opt.bqual_min = atoi(optarg);
        break;
      case 'Q':
        opt.min_passQ_frac = atof(optarg);
        break;
      case 'y':
        opt.max_N_filter = atoi(optarg);
        break;
      case 'x':
        opt.max_snv_filter = atoi(optarg);
        break;
      case 'c':
        opt.clustered_mut_cutoff = atoi(optarg);
        break;
      case 'd':
        opt.fragend_dist_filter = atoi(optarg);
        break;
      case 'F':
        opt.max_mismatch_frac = atof(optarg);
        break;
      case 'N':
        opt.max_N_frac = atof(optarg);
        break;
      case OPT_MAX_N_RATE_T2G:
        opt.max_N_frac_T2G = atof(optarg);
        break;
      case 'g':
        opt.min_fraglen = atoi(optarg);
        break;
      case 'G':
        opt.max_fraglen = atoi(optarg);
        break;
      case 'S':
        opt.load_supplementary = true;
        break;
      case '2':
        opt.load_secondary = true;
        break;
      case 'u':
        opt.load_unpair = true;
        break;
      case 'D':
        opt.load_duplicate = true;
        break;
      case '5':
        opt.filter_5endclip = true;
        break;
      case 'I':
        opt.allow_indel_near_snv = true;
        break;
      case 'R':
        opt.count_read = true;
        break;
      case 's':
        opt.sample = optarg;
        break;
      case 'V':
        opt.vcfs = cpputil::split(optarg, ",");
        break;
      case 'M':
        opt.maf_file = optarg;
        break;
      case 'r':
        opt.reference = optarg;
        break;
      case 'e':
        opt.error_prof_out = optarg;
        break;
      case 'k':
        opt.known_var_out = optarg;
        break;
      case 'C':
        opt.context_count = optarg;
        break;
      case OPT_QSCORE_PROF:
        opt.detail_qscore_prof = true;
        break;
      case OPT_READ_LEVEL_STAT:
        opt.read_level_stat = optarg;
        break;
      case OPT_CYCLE_LEVEL_STAT:
        opt.cycle_level_stat = optarg;
        break;
      case 'p':
        opt.pair_min_overlap = atoi(optarg);
        break;
      case 'A':
        opt.all_mutant_frags = true;
        break;
      default:accuracy_print_help();
        return 1;
    }
  } while (next_option != -1);

  return 0;
}


std::pair<int,int> NumEffBases(const cpputil::Segments& seg,
    const SeqLib::GenomicRegion* const gr,
    const SeqLib::RefGenome& ref,
    const string& chrname,
    const std::set<int>& blacklist,
    cpputil::ErrorStat& es,
    int minbq,
    std::pair<int, int>& nq0,
    bool count_context,
    bool count_overhang,
    bool N_is_valid){
  // blacklist represents a set of SNV positions that will not be counted in the error rate calculation
  int r1 = 0, r2 = 0, r1q0 = 0, r2q0 = 0;
  std::set<int> baseqblack;
  if (seg.size() == 1) {
    std::pair<int,int> range;
    const auto& br = seg.front();
    if (gr) {
      range = cpputil::GetBamOverlapQStartAndQStop(br, *gr);
    } else {
      range.first = br.AlignmentPosition();
      range.second = br.AlignmentEndPosition();
    }
    if (not br.PairedFlag() or br.FirstFlag()) {
      if (count_context)
        std::tie(r1,r1q0) = cpputil::CountValidBaseAndContextInMatchedBases(br, blacklist, chrname, ref, minbq, baseqblack, es, range.first, range.second, N_is_valid);
      else
        std::tie(r1, r1q0) = cpputil::CountValidBaseInMatchedBases(br, blacklist, minbq, baseqblack,range.first, range.second, N_is_valid);
    }
    else {
      if (count_context)
        std::tie(r2, r2q0) = cpputil::CountValidBaseAndContextInMatchedBases(br, blacklist, chrname, ref, minbq, baseqblack, es, range.first, range.second, N_is_valid);
      else
        std::tie(r2, r2q0) = cpputil::CountValidBaseInMatchedBases(br, blacklist, minbq, baseqblack, range.first, range.second, N_is_valid);
    }
  } else {
    std::pair<int, int> overlap_front, overlap_back;
    if (gr) {
      overlap_front = cpputil::GetBamOverlapQStartAndQStop(seg.front(), *gr);
      overlap_back = cpputil::GetBamOverlapQStartAndQStop(seg.back(), *gr);
    } else {
      overlap_front.first = seg.front().AlignmentPosition();
      overlap_front.second = seg.front().AlignmentEndPosition();
      overlap_back.first = seg.back().AlignmentPosition();
      overlap_back.second = seg.back().AlignmentEndPosition();
    }
    std::pair<int, int> range_front, range_back;
    if (count_overhang) {
      range_front = overlap_front;
      range_back = overlap_back;
    } else {
      std::tie(range_front, range_back) = cpputil::GetPairOverlapQStartAndQStop(seg.front(), seg.back());
      range_front.first = std::max(range_front.first, overlap_front.first);
      range_front.second = std::min(range_front.second, overlap_front.second);
      range_back.first = std::max(range_back.first, overlap_back.first);
      range_back.second = std::min(range_back.second, overlap_back.second);

    }
    if (seg.front().FirstFlag()) {
      size_t r1_nmask;
      if (count_context) {
        std::tie(r1, r1q0) = cpputil::CountValidBaseAndContextInMatchedBases(seg.front(), blacklist, chrname, ref, minbq, baseqblack, es, range_front.first, range_front.second, N_is_valid);
        r1_nmask = baseqblack.size();
        if (count_overhang) baseqblack.clear();// when count_overhang is true, treat R1 and R2 independently
        std::tie(r2, r2q0) = cpputil::CountValidBaseAndContextInMatchedBases(seg.back(), blacklist, chrname, ref, minbq, baseqblack, es, range_back.first, range_back.second, N_is_valid);
      }
      else {
        std::tie(r1, r1q0) = cpputil::CountValidBaseInMatchedBases(seg.front(), blacklist, minbq, baseqblack, range_front.first, range_front.second, N_is_valid);
        r1_nmask = baseqblack.size();
        if (count_overhang) baseqblack.clear();
        std::tie(r2, r2q0) = cpputil::CountValidBaseInMatchedBases(seg.back(), blacklist, minbq, baseqblack, range_back.first, range_back.second, N_is_valid);
      }
      if (not count_overhang) r1 -= baseqblack.size() - r1_nmask;
    } else {
      throw std::runtime_error("Read order wrong\n");
    }
  }
  nq0 = std::make_pair(r1q0, r2q0);
  return std::make_pair(r1, r2);
}

void CycleBaseCount(const cpputil::Segments& seg,
                    const SeqLib::GenomicRegion* const gr,
                    cpputil::ErrorStat& es) {
  auto res = std::make_pair(0,0);
  if (seg.size() == 1) {
    std::pair<int,int> range;
    if (gr) {
      range = cpputil::GetBamOverlapQStartAndQStop(seg.front(), *gr);
    } else {
      range.first = seg.front().AlignmentPosition();
      range.second = seg.front().AlignmentEndPosition();
    }
    if (seg.front().FirstFlag()) {
      cpputil::AddMatchedBasesToCycleCount(seg.front(), es.R1_q0_cov, es.R1_q30_cov, range.first, range.second);
    }
    else {
      cpputil::AddMatchedBasesToCycleCount(seg.front(), es.R2_q0_cov, es.R2_q30_cov, range.first, range.second);
    }
  } else {
    std::pair<int, int> overlap_front, overlap_back;
    if (gr) {
      overlap_front = cpputil::GetBamOverlapQStartAndQStop(seg.front(), *gr);
      overlap_back = cpputil::GetBamOverlapQStartAndQStop(seg.back(), *gr);
    } else {
      overlap_front.first = seg.front().AlignmentPosition();
      overlap_front.second = seg.front().AlignmentEndPosition();
      overlap_back.first = seg.back().AlignmentPosition();
      overlap_back.second = seg.back().AlignmentEndPosition();
    }
    if (seg.front().FirstFlag()) {
      cpputil::AddMatchedBasesToCycleCount(seg.front(), es.R1_q0_cov, es.R1_q30_cov, overlap_front.first, overlap_front.second);
      cpputil::AddMatchedBasesToCycleCount(seg.back(), es.R2_q0_cov, es.R2_q30_cov, overlap_back.first, overlap_back.second);
    } else {
      cpputil::AddMatchedBasesToCycleCount(seg.front(), es.R2_q0_cov, es.R2_q30_cov, overlap_front.first, overlap_front.second);
      cpputil::AddMatchedBasesToCycleCount(seg.back(), es.R1_q0_cov, es.R1_q30_cov, overlap_back.first, overlap_back.second);
    }
  }

}


int n_false_mut(vector<bool> v) {
  return count(v.begin(), v.end(), false);
}
int n_true_mut(vector<bool> v) {
  return count(v.begin(), v.end(), true);
}

void ErrorRateDriver(const vector<cpputil::Segments>& frag,
                    const int frag_numN,
                    const int nqpass,
                     const int olen,
                    const SeqLib::BamHeader& bamheader,
                    const SeqLib::RefGenome& ref,
                    const SeqLib::GenomicRegion* const gr,
                    const std::set<int> blacklist,
                    const AccuOptions& opt,
                    const cpputil::BCFReader& bcf_reader,
                    const cpputil::BCFReader& bcf_reader2,
                    const cpputil::MAFReader& mafr,
                    cpputil::PileHandler& pileup,
                    std::ofstream& ferr,
                    std::ofstream& known,
                    std::ofstream& readlevel,
                    cpputil::ErrorStat& errorstat) {


  for (auto seg : frag) { // a fragment may have multiple segs due to supplementary alignments
    //EOF filter
    if (seg.size() == 2) {
      ++errorstat.n_pass_filter_pairs;
      cpputil::TrimPairFromFragEnd(seg.front(), seg.back(), opt.fragend_dist_filter);
    } else if(seg.size() == 1) {
      ++errorstat.n_pass_filter_singles;
      cpputil::TrimSingleFromFragEnd(seg.front(), opt.fragend_dist_filter);
    }

    //for readlevel output
    //int r1_q0_den = 0, r2_q0_den = 0;
    //int r1_q0_nerror = 0, r2_q0_nerror = 0;
    int r1_nerror = 0, r2_nerror = 0;
    string chrname = seg.front().ChrName(bamheader);
    // get denominator
    std::pair<int, int> q0den(0, 0);
    auto den = NumEffBases(seg, gr, ref, chrname, blacklist, errorstat, opt.bqual_min, q0den, true, opt.count_read, false);
    int r1_den = den.first;
    int r2_den = den.second;
    std::string aux_output = std::to_string(frag_numN) + "\t" + std::to_string(nqpass) + "\t" + std::to_string(olen);
    errorstat.neval += r1_den + r2_den;
    errorstat.qcut_neval[0].first += q0den.first;
    errorstat.qcut_neval[0].second += q0den.second;

    if (!opt.cycle_level_stat.empty()) {
      CycleBaseCount(seg, gr, errorstat);
    }
    // first pass gets all variants in ROI
    std::map<cpputil::Variant, vector<cpputil::Variant>> var_vars;
    int rstart = 0;
    int rend = std::numeric_limits<int>::max();
    if (opt.count_read) {
      if (gr) {
        rstart = gr->pos1;
        rend = gr->pos2;
      }
    } else {
      std::tie(rstart,rend) = cpputil::GetPairOverlapRStartAndRStop(seg.front(), seg.back());
      if (gr) {
        rstart = std::max(rstart, gr->pos1);
        rend = std::min(rend, gr->pos2);
      }
    }

    for (auto &s : seg) {
      auto vars = cpputil::GetVar(s, bamheader, ref);
      for (auto &var : vars) {
        if (var.contig_start >= rstart && var.contig_start < rend)
          var_vars[var].push_back(var);
      }
    }

    // Currenlty, always assume the variants are concordance between R1 and R2
    //TODO: if count_read == true, allow single variants in the overhang regions.
    std::vector<std::vector<cpputil::Variant>> refined_vars;
    refined_vars.reserve(var_vars.size());
    //Consolidate SNPs which has too purposes:
    // 1. change the variant quality of both reads to the lowest one so that they can be filtered of keep as a whole
    // 2. break doublets and etc. if not all bases are satisfying baseq cutoff.
    for (const auto& it: var_vars) {
      if (it.first.isIndel()) {
        refined_vars.push_back(it.second);
        continue;
      }
      if(it.second.size() == 1) {
        if (!it.first.isMNV() or it.second[0].var_qual >= opt.bqual_min) {
          refined_vars.push_back(it.second);
        } else {
          auto vars = var_atomize(it.second[0]);
          for (auto& var : vars) {
            refined_vars.push_back({var});
          }
        }
      } else {
        auto vars = cpputil::snppair_consolidate(it.second[0], it.second[1], opt.bqual_min);
        refined_vars.insert(refined_vars.end(), vars.begin(), vars.end());
      }
    }
    // sort by var type first so that indels are in front
//    sort(refined_vars.begin(), refined_vars.end(), [](const auto& lhs, const auto& rhs) {
//      return lhs[0].Type() < rhs[0].Type();
//    });
    for (const auto& readpair_var: refined_vars) {
      auto var = cpputil::squash_vars(readpair_var);
      //int known_indel_len = 0;
      vector<bool> real_muts;
      bool found = false;
      if (var.Type() == "SNV") {
        real_muts.resize(var.alt_seq.size(), false);
      }
      if (bcf_reader.IsOpen()) {
         found = cpputil::search_var_in_database(bcf_reader, var, known, "PRI_VCF", real_muts, true, opt.bqual_min, opt.all_mutant_frags);
      }
      if (bcf_reader2.IsOpen() && not found) {
        int maskbyvcf1 = n_true_mut(real_muts);
        found = cpputil::search_var_in_database(bcf_reader2, var, known, "SEC_VCF", real_muts, true, opt.bqual_min, opt.all_mutant_frags);
        if (var.Type() == "SNV" and var.var_qual >= opt.bqual_min)
          errorstat.nerror_masked_by_vcf2 += (n_true_mut(real_muts) - maskbyvcf1) * var.read_count;
      }
      if (mafr.IsOpen() && not found) {
        found = cpputil::search_var_in_database(mafr, var, known, "MAF", real_muts, true, opt.bqual_min, opt.all_mutant_frags);
      }
      if (not found) { // error site
        // mutant_families.txt
        int nerr = 0, q0nerr = 0;
        std::vector<cpputil::Variant> avars;
        if (n_true_mut(real_muts) > 0) { // partially wrong
          auto tmpvar = cpputil::var_atomize(var);
          for (unsigned ai = 0; ai < real_muts.size(); ++ai) {
            if (not real_muts[ai]) {
              avars.push_back(tmpvar[ai]);
            }
          }
        } else { // fully wrong
          avars.push_back(var);
        }
        for (unsigned ai = 0; ai < avars.size(); ++ai) {
          if (avars[ai].isIndel()) {
            ferr << avars[ai] << '\t' << aux_output <<'\n';
          } else {
            int germ_support = 0, germ_cov = std::numeric_limits<int>::max();
            if (opt.germline_bam.size() > 1) {
              germ_cov = cpputil::ScanAllele(&pileup, avars[ai].contig, avars[ai].contig_start, avars[ai].alt_seq[0], true, germ_support);
            }
            if (germ_cov < opt.germline_mindepth) {
              errorstat.low_germ_depth += avars[ai].read_count;
            } else if (germ_support > 0) {
              errorstat.seen_in_germ += avars[ai].read_count;
            } else {
              if (avars[ai].var_qual >= opt.bqual_min &&
              (opt.count_read || seg.size() == 1 || readpair_var.size() == 2 )) {
                if (not opt.allow_indel_near_snv && cpputil::IndelLen(seg.front())> 0) {
                  errorstat.mismatch_filtered_by_indel += avars[ai].read_count;
                } else {
                  if (avars[ai].MutType() == "T>G" and (float) frag_numN > olen * opt.max_N_frac_T2G) {
                    errorstat.lowconf_t2g += avars[ai].read_count;
                  } else {
                    nerr += avars[ai].read_count * avars[ai].alt_seq.size();
                    ferr << avars[ai] << '\t' << aux_output <<'\n';
                  }
                }
              } else { // q0 erorr rate
                q0nerr += avars[ai].read_count * avars[ai].alt_seq.size();
              }
            }
          }
        }

        errorstat.nerror += nerr;
        if (var.read_count == 2) {
          errorstat.qcut_nerrors[0].first += q0nerr;
          errorstat.qcut_nerrors[0].second += q0nerr;
        } else {
          if (var.first_of_pair)
            errorstat.qcut_nerrors[0].first += q0nerr;
          else
            errorstat.qcut_nerrors[0].second += q0nerr;
        }

        // other error profiles
        if (var.Type() == "SNV") {
          if (!opt.read_level_stat.empty()) {
            if (var.var_qual >= opt.bqual_min && (seg.size() == 1 || readpair_var.size() == 2)) {
              if (var.read_count == 2) {
                r1_nerror += nerr;
                r2_nerror += nerr;
              }
              else {
                if (var.first_of_pair ) r1_nerror += nerr;
                else r2_nerror += nerr;
              }
            }
          }
          //per cycle profile
          if (!opt.cycle_level_stat.empty()) {
            int tpos = var.alt_start < 0 ?  abs(var.alt_start + 1): var.alt_start;
            var.first_of_pair ? errorstat.R1_q0_error[tpos] += nerr : errorstat.R2_q0_error[tpos] += nerr;
            if (var.var_qual >= 30) {
              var.first_of_pair ? errorstat.R1_q30_error[tpos] += nerr: errorstat.R2_q30_error[tpos] += nerr;
            }
          }
        } //end other profiles
      }
    }

    if (readlevel.is_open()) {
      readlevel << seg.front().Qname() << '\t'
                << r1_nerror << '\t'
                << r2_nerror << '\t'
                << r1_den  << '\t'
                << r2_den << '\t'
                << frag_numN << '\t'
                << abs(seg.front().InsertSize()) << '\t'
                << olen << '\n';
    }
  } // end for
  return;
}

int codec_accuracy(int argc, char ** argv) {
  AccuOptions opt;
  int parse_ret =  accuracy_parse_options(argc, argv, opt);
  if (parse_ret) return 1;
  if (argc == 1) {
    accuracy_print_help();
    return 1;
  }
  SeqLib::RefGenome ref;
  ref.LoadIndex(opt.reference);
  std::ofstream stat(opt.accuracy_stat);
  std::ofstream ferr(opt.error_prof_out);
  std::ofstream context(opt.context_count);

  cpputil::BCFReader bcf_reader;
  cpputil::BCFReader bcf_reader2;
  if (!opt.vcfs.empty()) {
    bcf_reader.Open(opt.vcfs[0].c_str(), opt.sample);
    if (opt.vcfs.size() == 2) {
      bcf_reader2.Open(opt.vcfs[1].c_str());
    }
    else if (opt.vcfs.size() > 2){
      std::cerr << "Cannot read more than 2 vcfs currently\n";
      return 1;
    }
  }
  cpputil::PileHandler pileup;
  if (opt.germline_bam.size() > 1) {
    pileup = cpputil::PileHandler(opt.germline_bam, opt.germline_minmapq);
  }
  std::ofstream known;
  std::ofstream readlevel;
  std::ofstream cyclelevel;
  string error_profile_header =
      "chrom\tref_pos\tref\talt\ttype\tdist_to_fragend\tsnv_base_qual\tread_count\tread_name\tfamily_size\tnumN\tnumQ30\tolen";
  ferr << error_profile_header << std::endl;
  if (not opt.known_var_out.empty()) {
    known.open(opt.known_var_out);
    string known_var_header =
        "chrom\tref_pos\tref\talt\ttype\tread_pos\tsnv_base_qual\tfirst_of_pair\tread_name\tevidence";
    known << known_var_header << std::endl;
  }
  if (not opt.read_level_stat.empty()) {
    readlevel.open(opt.read_level_stat);
    string read_level_header =
        "read_name\tR1_nerror\tR2_nerror\tR1_efflen\tR2_efflen\tfrag_numN\tfraglen\toverlap_len";
    readlevel << read_level_header << std::endl;
  }
  if (not opt.cycle_level_stat.empty()) {
    cyclelevel.open(opt.cycle_level_stat);
    cyclelevel << "cycle\tR1_q0_error\tR1_q0_cov\tR1_q30_error\tR1_q30_cov\t"
            "R2_q0_error\tR2_q0_cov\tR2_q30_error\tR2_q30_cov\t"
            "R1_q0_erate\tR1_q30_erate\tR2_q0_erate\tR2_q30_erate\n";
  }
  cpputil::InsertSeqFactory isf(opt.bam, opt.mapq, opt.load_supplementary, opt.load_secondary, opt.load_duplicate, false);
//  SeqLib::BamWriter trim_bam_writer;
//  if (!opt.trim_bam.empty()) {
//    trim_bam_writer.SetHeader(isf.bamheader());
//    trim_bam_writer.Open(opt.trim_bam);
//    trim_bam_writer.WriteHeader();
//  }
  cpputil::MAFReader mafr;
  if (!opt.maf_file.empty()) {
    mafr.Open(opt.maf_file);
  }
  const int L = 250;
  auto errorstat = cpputil::ErrorStat(L);
  if (opt.detail_qscore_prof) {
    errorstat = cpputil::ErrorStat({0,10,20,30}, L);
  }

  cpputil::TargetLayout tl(isf.bamheader(), opt.bed_file);
  // SeqLib::GenomicRegion gr;
  //while(tl.NextRegion(gr)) {
  vector<vector<cpputil::Segments>> chunk;
  std::unordered_set<std::string> pass_qnames;
  int last_chrom = -1;
  cpputil::UniqueQueue readpair_cnt(100000);
  cpputil::UniqueQueue failed_cnt(100000);
  for (int i = 0; i < tl.NumRegion(); ++i) {
    const auto& gr = tl[i];
    if (gr.chr != last_chrom) {
      last_chrom = gr.chr;
      readpair_cnt.clearQueue();
      failed_cnt.clearQueue();
    }
    if (i % 100000 == 0) {
      auto timenow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      std::cerr << i + 1 << " region processed. Last position: " << gr << std::ctime(&timenow) << std::endl;
    }
    if (isf.ReadByRegion(cpputil::ArePEAlignmentOverlapAtLeastK, gr, chunk, opt.pair_min_overlap, "", opt.load_unpair)) {
      std::set<int32_t> blacklist;
      if (bcf_reader.IsOpen()) {
        bcf_reader.vcf_to_blacklist_snv(gr, isf.bamheader(), blacklist);
      }
      for (const vector<cpputil::Segments>& frag : chunk) {
        readpair_cnt.add(frag[0][0].Qname());
        if (failed_cnt.exist(frag[0][0].Qname())) {
          //std::cerr << frag[0][0] << " already faile" << std::endl;
          continue;
        }
        int frag_numN, nqpass, olen;
        int fail = cpputil::FailFilter(frag, isf.bamheader(), ref, opt, errorstat, isf.IsPairEndLib() & !opt.load_unpair, frag_numN, nqpass, olen);
        if (fail) {
          ++errorstat.discard_frag_counter;
          //std::cerr << fail << ": " << frag[0].size() << ", " << frag[0][0].Qname() << std::endl;
          failed_cnt.add(frag[0][0].Qname());
        }
        else {
          ErrorRateDriver(frag,
                          frag_numN, nqpass, olen,
                          isf.bamheader(),
                          ref,
                          &gr,
                          blacklist,
                          opt,
                          bcf_reader,
                          bcf_reader2,
                          mafr,
                          pileup,
                          ferr,
                          known,
                          readlevel,
                          errorstat);
        }
      } // end for
    } //end if
  } //end for

  std::cerr << "All region processed \n";

  vector<string> header = {"qcutoff",
                           "n_trim",
                                     "n_bases_eval",
                                     "n_A_eval",
                                     "n_C_eval",
                                     "n_G_eval",
                                     "n_T_eval",
                                     "n_errors",
                                     "erate",
                                     "n_errors_masked_by_vcf2",
                                     "n_totalpairs",
                                     "n_pass_filter_pair_seg",
                                     "n_pass_filter_single_seg",
                                     "n_filtered_total",
                                     "n_filtered_smallfrag",
                                     "n_filtered_scalip",
                                     "n_filtered_Nrate",
                                     "n_filtered_q30rate",
                                     "n_filtered_largefrag",
                                     "n_filtered_edit",
                                     "n_filtered_clustered",
                                     "nmut_germ_lowdepth",
                                     "nmut_germ_seen",
                                     "nmut_mismatch_filtered_by_indel",
                                     "nmut_t2g_low_conf",
                                     "q30_rate"};

  for (const auto& duo : errorstat.qcut_nerrors) {
    header.push_back("q" + std::to_string(duo.first) + "_n_bases_eval" );
    header.push_back("q" + std::to_string(duo.first) + "_n_errors" );
    header.push_back("q" + std::to_string(duo.first) + "_erate" );
    header.push_back("q" + std::to_string(duo.first) + "_R1_n_bases_eval" );
    header.push_back("q" + std::to_string(duo.first) + "_R1_n_errors" );
    header.push_back("q" + std::to_string(duo.first) + "_R1_erate" );
    header.push_back("q" + std::to_string(duo.first) + "_R2_n_bases_eval" );
    header.push_back("q" + std::to_string(duo.first) + "_R2_n_errors" );
    header.push_back("q" + std::to_string(duo.first) + "_R2_erate" );
  }

  stat << cpputil::join(header, "\t") << std::endl;
  stat << opt.bqual_min << '\t'
       << opt.fragend_dist_filter << '\t'
       << errorstat.neval << '\t'
      << errorstat.base_counter['A'] << '\t'
      << errorstat.base_counter['C'] << '\t'
      << errorstat.base_counter['G'] << '\t'
      << errorstat.base_counter['T'] << '\t'
      << errorstat.nerror << '\t'
       << (float) errorstat.nerror / errorstat.neval << '\t'
       << errorstat.nerror_masked_by_vcf2 << '\t'
       << readpair_cnt.NumAdded() << '\t'
       << errorstat.n_pass_filter_pairs << '\t'
       << errorstat.n_pass_filter_singles << '\t'
       << errorstat.discard_frag_counter << '\t'
       << errorstat.n_filtered_smallfrag << '\t'
      << errorstat.n_filtered_sclip << '\t'
      << errorstat.n_filtered_Nrate << '\t'
      << errorstat.n_filtered_q30rate << '\t'
       << errorstat.n_filtered_largefrag << '\t'
       << errorstat.n_filtered_edit << '\t'
      << errorstat.n_filtered_clustered << '\t'
       << errorstat.low_germ_depth << '\t'
       << errorstat.seen_in_germ << '\t'
       << errorstat.mismatch_filtered_by_indel << '\t'
       << errorstat.lowconf_t2g << '\t'
       << (float) errorstat.neval / (errorstat.qcut_neval[0].first + errorstat.qcut_neval[0].second)  << '\t';

  int ii = 0;
  for (const auto& qcut : errorstat.cutoffs) {
    ++ii;
    int64_t r1_den = errorstat.qcut_neval[qcut].first;
    int64_t r2_den = errorstat.qcut_neval[qcut].second;
    int64_t r1_num = errorstat.qcut_nerrors[qcut].first;
    int64_t r2_num = errorstat.qcut_nerrors[qcut].second;
    stat << r1_den + r2_den  << '\t';
    stat << r1_num + r2_num << '\t';
    stat << (float) (r1_num + r2_num) / (r1_den + r2_den) << '\t';
    stat << r1_den << '\t';
    stat << r1_num << '\t';
    stat << (float) r1_num / r1_den<< '\t';
    stat << r2_den << '\t';
    stat << r2_num << '\t';
    stat << (float) r2_num / r2_den;
    if (ii == errorstat.cutoffs.size())  stat << '\n';
    else stat << '\t';
  }

  for (const auto& it : errorstat.triplet_counter) {
    context << it.first << "\t" << it.second << std::endl;
  }

  for (const auto& it : errorstat.doublet_counter) {
    context << it.first << "\t" << it.second << std::endl;
  }

  if (!opt.cycle_level_stat.empty()) {
    for (int i = 0; i < L; ++i) {
     cyclelevel  << i << '\t';
     cyclelevel  << errorstat.R1_q0_error[i] << '\t' << errorstat.R1_q0_cov[i] << '\t';
     cyclelevel  << errorstat.R1_q30_error[i] << '\t' << errorstat.R1_q30_cov[i] << '\t';
     cyclelevel  << errorstat.R2_q0_error[i] << '\t' << errorstat.R2_q0_cov[i] << '\t';
     cyclelevel  << errorstat.R2_q30_error[i] << '\t' << errorstat.R2_q30_cov[i] << '\t';
     cyclelevel  << (float) errorstat.R1_q0_error[i] / errorstat.R1_q0_cov[i] << '\t';
     cyclelevel  << (float) errorstat.R1_q30_error[i] / errorstat.R1_q30_cov[i] << '\t';
     cyclelevel  << (float) errorstat.R2_q0_error[i] / errorstat.R2_q0_cov[i] << '\t';
     cyclelevel  << (float) errorstat.R2_q30_error[i] / errorstat.R2_q30_cov[i] << '\n';
    }
  }
  return 0;
}
