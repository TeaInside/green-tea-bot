import TableUserStatistic from "./TableUserStatistic"
import TableWordStatistic from "./TableWordStatistic"

export default function TableStatistic() {
    return (
        <div className="grid grid-col row-span-2 gap-y-10">
            <TableUserStatistic />
            <TableWordStatistic />
        </div>
    )
}