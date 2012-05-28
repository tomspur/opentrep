#ifndef __OPENTREP_BOM_RESULTCOMBINATION_HPP
#define __OPENTREP_BOM_RESULTCOMBINATION_HPP

// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// OpenTrep
#include <opentrep/OPENTREP_Types.hpp>
#include <opentrep/bom/BomAbstract.hpp>
#include <opentrep/bom/ResultHolderList.hpp>

// Forward declarations
namespace Xapian {
  class Database;
}

namespace OPENTREP {

  /**
   * @brief Class wrapping functions on a list of ResultHolder objects.
   */
  class ResultCombination : public BomAbstract {
    friend class FacResultCombination;
  public:
    // ////////////// Getters /////////////
    /**
     * Get the query string.
     */
    const TravelQuery_T& getQueryString() const {
      return _travelQuery;
    }

    /**
     * Retrieve the list of result objects.
     */
    const ResultHolderList_T& getResultHolderList() const {
      return _resultHolderList;
    }

    /**
     * Get a reference on the best matching ResultHolder object.
     * If there is no such object, the method throws a
     * NoBestMatchingResultHolderException exception.
     */
    const ResultHolder& getBestMatchingResultHolder() const;


  public:
    // /////////// Business methods ///////////
    /**
     * Choose the best matching ResultHolder object from the underlying list.
     *
     * @return bool Whether or not a best matching ResultHolder exists.
     */
    bool chooseBestMatchingResultHolder();


  public:
    // /////////// Display support methods /////////
    /**
     * Dump a Business Object into an output stream.
     *
     * @param ostream& the output stream.
     */
    void toStream (std::ostream&) const;

    /**
     * Read a Business Object from an input stream.
     *
     * @param istream& the input stream.
     */
    void fromStream (std::istream&);

    /**
     * Get the serialised version of the Business Object.
     */
    std::string toString() const;
    
    /**
     * Get a string describing the whole key (differentiating two objects
     * at any level).
     */
    std::string describeKey() const;

    /**
     * Get a string describing the short key (differentiating two objects
     * at the same level).
     */
    std::string describeShortKey() const;


  private:
    // ////////////// Constructors and Destructors /////////////
    /**
     * Main constructor.
     */
    ResultCombination (const TravelQuery_T&);
    /**
     * Destructor.
     */
    ~ResultCombination ();
    /**
     * Initialise (reset the list of documents).
     */
    void init ();
  private:
    /**
     * Default constructor. It should not be used.
     */
    ResultCombination();
    /**
     * Copy constructor. It should not be used.
     */
    ResultCombination (const ResultCombination&);
    
    
  private:
    // /////////////// Attributes ////////////////
    /**
     * Query string having generated the list of document.
     */
    const TravelQuery_T _travelQuery;

    /**
     * List of ResultHolder objects.
     */
    ResultHolderList_T _resultHolderList;

    /**
     * Best matching ResultHolder object.
     */
    const ResultHolder* _bestMatchingResultHolder;
  };

}
#endif // __OPENTREP_BOM_RESULTCOMBINATION_HPP
