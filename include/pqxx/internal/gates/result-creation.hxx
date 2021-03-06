#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE result_creation : callgate<const result>
{
  friend class pqxx::connection_base;
  friend class pqxx::pipeline;

  result_creation(reference x) : super(x) {}

  static result create(internal::pq::PGresult *rhs, const std::string &query)
  {
    return result(rhs, query);
  }

  void CheckStatus() const { return home().CheckStatus(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
